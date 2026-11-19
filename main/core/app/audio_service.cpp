// AudioService: capture -> encode -> send, composed from injected ports.

#include "core/app/audio_service.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>

#include "core/app/net_manager.hpp"
#include "core/audio/AudioPipeline.hpp"
#include "interface.hpp"

#include "audio_protocol.hpp"      // protocol::IoT::AudioPacket / encodeAudio
#include "turn_on_audio.hpp"       // protocol::IoT::client::encodeRecordAck

namespace AudioService {

namespace {

constexpr uint32_t kLoopDelayMs   = 10;
constexpr uint32_t kTaskStack     = 39152;
constexpr int      kTaskPriority  = 5;
constexpr int      kTaskCore      = 1;
constexpr int      kStopTimeoutMs = 500;

enum class RunState { Stopped, Running, Stopping };

struct AudioState {
    // Guards task-handle publication vs. the task clearing it on another core.
    std::mutex mutex;
    std::atomic<core::ITaskRunner::TaskId> task{nullptr};
    std::atomic<bool> running{false};
    std::atomic<RunState> run_state{RunState::Stopped};
    std::atomic<uint32_t> request_id{0};   // RecordControl.requestId
};

AudioState& state() {
    static AudioState s;
    return s;
}

core::IAppServices*& services() {
    static core::IAppServices* s = nullptr;
    return s;
}

void send_ack(core::IAppServices& svc, protocol::IoT::RecordCommand cmd,
              uint32_t request_id, bool ok, const char* error = "") {
    Network::send(protocol::IoT::client::encodeRecordAck(
        cmd, svc.clock.nowUnixMs(), request_id, ok, error));
}

void clear_task_handle() {
    auto& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    s.task.store(nullptr);
    s.run_state.store(RunState::Stopped);
}

void audio_task(void* /*arg*/) {
    core::IAppServices& svc = *services();
    const uint32_t request_id = state().request_id.load();

    svc.log.logf(core::LogLevel::Info, "Audio task created & initializing");

    if (!svc.audio_in.open()) {
        svc.log.logf(core::LogLevel::Error, "Audio input open failed");
        send_ack(svc, protocol::IoT::RecordCommand::START, request_id, false,
                 "audio input open failed");
        clear_task_handle();
        svc.tasks.exitCurrentTask();
        return;
    }

    if (!svc.audio_encoder.init()) {
        svc.log.logf(core::LogLevel::Error, "Audio encoder init failed");
        svc.audio_in.close();   // release the driver + PSRAM DMA buffer
        send_ack(svc, protocol::IoT::RecordCommand::START, request_id, false,
                 "opus encoder init failed");
        clear_task_handle();
        svc.tasks.exitCurrentTask();
        return;
    }

    // Pipeline is really running: only now report START success to the server.
    send_ack(svc, protocol::IoT::RecordCommand::START, request_id, true);

    core::AudioPipeline pipeline(
        svc.audio_in, svc.audio_encoder, svc.clock, svc.log,
        [&svc](const core::Bytes& compressed, uint32_t timestamp_ms) {
            protocol::IoT::AudioPacket packet;
            packet.timestampMs = static_cast<int64_t>(timestamp_ms);
            packet.durationMs = svc.audio_encoder.frameDurationMs();
            packet.payload = compressed;
            packet.codec = 1;   // 1 = Opus
            packet.sampleRate = svc.audio_encoder.sampleRate();
            packet.channels = svc.audio_encoder.channelCount();
            Network::send(protocol::IoT::client::encodeAudio(packet));
        });

    while (state().running.load()) {
        // The I2S read inside step() blocks ~1 frame and yields, so only delay
        // on failure; otherwise the loop would fall behind the 20 ms frame rate.
        if (!pipeline.step()) {
            svc.tasks.delayMs(kLoopDelayMs);
        }
    }

    svc.audio_in.close();
    svc.log.logf(core::LogLevel::Info, "Exiting audio loop: releasing drivers");
    clear_task_handle();
    svc.tasks.exitCurrentTask();
}

} // namespace

void start(core::IAppServices& svc, uint32_t request_id) {
    services() = &svc;
    auto& s = state();

    // If a previous stop is still winding down, wait for it (outside the
    // mutex; the exiting task needs it to clear its handle).
    if (s.run_state.load() == RunState::Stopping) {
        int waited = 0;
        while (s.task.load() != nullptr && waited < kStopTimeoutMs) {
            svc.tasks.delayMs(kLoopDelayMs);
            waited += kLoopDelayMs;
        }
    }

    // Hold the lock across publish+store: a fast-exiting task cannot erase the
    // handle before start() stores it (cross-core startup race).
    std::lock_guard<std::mutex> lock(s.mutex);
    if (s.run_state.load() != RunState::Stopped) {
        svc.log.logf(core::LogLevel::Warn, "Audio already running; acking START");
        send_ack(svc, protocol::IoT::RecordCommand::START, request_id, true);
        return;
    }

    s.request_id.store(request_id);
    s.running.store(true);
    s.run_state.store(RunState::Running);
    s.task.store(svc.tasks.start("audio_proc_task", kTaskStack, kTaskPriority, kTaskCore,
                                 audio_task, nullptr));

    if (s.task.load() == nullptr) {
        svc.log.logf(core::LogLevel::Error, "Failed to create audio task!");
        s.run_state.store(RunState::Stopped);
        s.running.store(false);
        send_ack(svc, protocol::IoT::RecordCommand::START, request_id, false,
                 "task create failed");
        return;
    }
    // START ack is sent by audio_task once open()/init() actually succeeded.
}

void stop(core::IAppServices& svc, uint32_t request_id, bool ack) {
    services() = &svc;
    auto& s = state();

    if (s.run_state.load() == RunState::Stopped) {
        svc.log.logf(core::LogLevel::Warn, "Audio already stopped");
        if (ack) {
            send_ack(svc, protocol::IoT::RecordCommand::STOP, request_id, true);
        }
        return;
    }

    svc.log.logf(core::LogLevel::Info, "Stopping audio task...");
    s.run_state.store(RunState::Stopping);
    s.running.store(false);

    int waited = 0;
    while (s.task.load() != nullptr && waited < kStopTimeoutMs) {
        svc.tasks.delayMs(kLoopDelayMs);
        waited += kLoopDelayMs;
    }

    if (s.task.load() != nullptr) {
        // Never vTaskDelete a task that may hold NetState::mutex or heap locks.
        // The task will finish on its own; run_state stays Stopping until then,
        // and start() waits for it before creating a new task.
        svc.log.logf(core::LogLevel::Error,
                     "Audio task did not exit within %d ms; leaving it to finish",
                     kStopTimeoutMs);
        return;
    }

    s.run_state.store(RunState::Stopped);
    svc.log.logf(core::LogLevel::Info, "Audio task stopped");
    if (ack) {
        send_ack(svc, protocol::IoT::RecordCommand::STOP, request_id, true);
    }
}

bool is_running() {
    return state().run_state.load() != RunState::Stopped;
}

} // namespace AudioService
