// AudioService: capture -> encode -> send, composed from injected ports.

#include "core/app/audio_service.hpp"

#include "core/audio/AudioPipeline.hpp"
#include "interface.hpp"

#include "audio_protocol.hpp"      // protocol::IoT::AudioPacket / encodeAudio
#include "turn_on_audio.hpp"       // protocol::IoT::client::encodeRecordAck

namespace core::app {

namespace {

constexpr uint32_t kLoopDelayMs   = 10;
constexpr uint32_t kTaskStack     = 39152;
constexpr int      kTaskPriority  = 5;
constexpr int      kTaskCore      = 1;
constexpr int      kStopTimeoutMs = 500;

void sendAck(INetwork& network, IClock& clock, protocol::IoT::RecordCommand cmd,
             uint32_t request_id, bool ok, const char* error = "") {
    network.send(protocol::IoT::client::encodeRecordAck(
        cmd, clock.nowUnixMs(), request_id, ok, error));
}

} // namespace

AudioService::AudioService(IAudioSource& input, IAudioEncoder& encoder, IClock& clock,
                           ITaskRunner& tasks, ILogger& logger, INetwork& network)
    : input_(input),
      encoder_(encoder),
      clock_(clock),
      tasks_(tasks),
      logger_(logger),
      network_(network) {}

AudioService::~AudioService() {
    // Signal the loop only; the owner must stop() while the task runner lives.
    running_.store(false);
}

void AudioService::taskEntry(void* arg) {
    static_cast<AudioService*>(arg)->taskLoop();
}

void AudioService::clearTaskHandle() {
    std::lock_guard<std::mutex> lock(mutex_);
    task_.store(nullptr);
    run_state_.store(RunState::Stopped);
}

void AudioService::taskLoop() {
    const uint32_t request_id = request_id_.load();

    logger_.logf(LogLevel::Info, "Audio task created & initializing");

    if (!input_.open()) {
        logger_.logf(LogLevel::Error, "Audio input open failed");
        sendAck(network_, clock_, protocol::IoT::RecordCommand::START, request_id,
                false, "audio input open failed");
        clearTaskHandle();
        tasks_.exitCurrentTask();
        return;
    }

    if (!encoder_.init()) {
        logger_.logf(LogLevel::Error, "Audio encoder init failed");
        input_.close();   // release the driver + PSRAM DMA buffer
        sendAck(network_, clock_, protocol::IoT::RecordCommand::START, request_id,
                false, "opus encoder init failed");
        clearTaskHandle();
        tasks_.exitCurrentTask();
        return;
    }

    // Pipeline is really running: only now report START success to the server.
    sendAck(network_, clock_, protocol::IoT::RecordCommand::START, request_id, true);

    AudioPipeline pipeline(
        input_, encoder_, clock_, logger_,
        [this](const Bytes& compressed, uint32_t timestamp_ms) {
            protocol::IoT::AudioPacket packet;
            packet.timestampMs = static_cast<int64_t>(timestamp_ms);
            packet.durationMs = encoder_.frameDurationMs();
            packet.payload = compressed;
            packet.codec = 1;   // 1 = Opus
            packet.sampleRate = encoder_.sampleRate();
            packet.channels = encoder_.channelCount();
            network_.send(protocol::IoT::client::encodeAudio(packet));
        });

    while (running_.load()) {
        // The I2S read inside step() blocks ~1 frame and yields, so only delay
        // on failure; otherwise the loop would fall behind the 20 ms frame rate.
        if (!pipeline.step()) {
            tasks_.delayMs(kLoopDelayMs);
        }
    }

    input_.close();
    logger_.logf(LogLevel::Info, "Exiting audio loop: releasing drivers");
    clearTaskHandle();
    tasks_.exitCurrentTask();
}

void AudioService::start(uint32_t request_id) {
    // If a previous stop is still winding down, wait for it (outside the
    // mutex; the exiting task needs it to clear its handle).
    if (run_state_.load() == RunState::Stopping) {
        int waited = 0;
        while (task_.load() != nullptr && waited < kStopTimeoutMs) {
            tasks_.delayMs(kLoopDelayMs);
            waited += kLoopDelayMs;
        }
    }

    // Hold the lock across publish+store: a fast-exiting task cannot erase the
    // handle before start() stores it (cross-core startup race).
    std::lock_guard<std::mutex> lock(mutex_);
    if (run_state_.load() != RunState::Stopped) {
        logger_.logf(LogLevel::Warn, "Audio already running; acking START");
        sendAck(network_, clock_, protocol::IoT::RecordCommand::START, request_id, true);
        return;
    }

    request_id_.store(request_id);
    running_.store(true);
    run_state_.store(RunState::Running);
    task_.store(tasks_.start("audio_proc_task", kTaskStack, kTaskPriority, kTaskCore,
                             taskEntry, this));

    if (task_.load() == nullptr) {
        logger_.logf(LogLevel::Error, "Failed to create audio task!");
        run_state_.store(RunState::Stopped);
        running_.store(false);
        sendAck(network_, clock_, protocol::IoT::RecordCommand::START, request_id,
                false, "task create failed");
        return;
    }
    // START ack is sent by taskLoop once open()/init() actually succeeded.
}

void AudioService::stop(uint32_t request_id, bool ack) {
    if (run_state_.load() == RunState::Stopped) {
        logger_.logf(LogLevel::Warn, "Audio already stopped");
        if (ack) {
            sendAck(network_, clock_, protocol::IoT::RecordCommand::STOP, request_id, true);
        }
        return;
    }

    logger_.logf(LogLevel::Info, "Stopping audio task...");
    run_state_.store(RunState::Stopping);
    running_.store(false);

    int waited = 0;
    while (task_.load() != nullptr && waited < kStopTimeoutMs) {
        tasks_.delayMs(kLoopDelayMs);
        waited += kLoopDelayMs;
    }

    if (task_.load() != nullptr) {
        // Never vTaskDelete a task that may hold heap/lock state. The task will
        // finish on its own; run_state stays Stopping until then, and start()
        // waits for it before creating a new task.
        logger_.logf(LogLevel::Error,
                     "Audio task did not exit within %d ms; leaving it to finish",
                     kStopTimeoutMs);
        return;
    }

    run_state_.store(RunState::Stopped);
    logger_.logf(LogLevel::Info, "Audio task stopped");
    if (ack) {
        sendAck(network_, clock_, protocol::IoT::RecordCommand::STOP, request_id, true);
    }
}

bool AudioService::isRunning() const {
    return run_state_.load() != RunState::Stopped;
}

} // namespace core::app
