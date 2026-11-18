// AudioService: capture -> encode -> send, composed from injected ports.

#include "core/app/audio_service.hpp"

#include <atomic>
#include <cstdint>

#include "core/app/net_manager.hpp"
#include "core/audio/AudioPipeline.hpp"
#include "interface.hpp"

#include "audio_protocol.hpp"      // protocol::IoT::client::encodeAudio
#include "turn_on_audio.hpp"       // protocol::IoT::client::encodeRecordAck

namespace AudioService {

namespace {

constexpr uint32_t kLoopDelayMs   = 10;
constexpr uint32_t kTaskStack     = 39152;
constexpr int      kTaskPriority  = 5;
constexpr int      kTaskCore      = 1;
constexpr int      kStopTimeoutMs = 200;

struct AudioState {
    std::atomic<core::ITaskRunner::TaskId> task{nullptr};
    std::atomic<bool> running{false};
};

AudioState& state() {
    static AudioState s;
    return s;
}

core::IAppServices*& services() {
    static core::IAppServices* s = nullptr;
    return s;
}

void audio_task(void* /*arg*/) {
    core::IAppServices& svc = *services();
    svc.log.logf(core::LogLevel::Info, "Audio task created & initializing");

    if (!svc.audio_in.open() || !svc.audio_encoder.init()) {
        svc.log.logf(core::LogLevel::Error, "Audio pipeline init failed");
        state().task.store(nullptr);
        svc.tasks.exitCurrentTask();
        return;
    }

    core::AudioPipeline pipeline(
        svc.audio_in, svc.audio_encoder, svc.clock, svc.log,
        [](const core::Bytes& compressed, uint32_t timestamp_ms) {
            Network::send(protocol::IoT::client::encodeAudio(compressed, timestamp_ms));
        });

    while (state().running.load()) {
        pipeline.step();
        svc.tasks.delayMs(kLoopDelayMs);
    }

    svc.audio_in.close();
    svc.log.logf(core::LogLevel::Info, "Exiting audio loop: releasing drivers");
    state().task.store(nullptr);
    svc.tasks.exitCurrentTask();
}

} // namespace

void start(core::IAppServices& svc) {
    services() = &svc;
    auto& s = state();

    if (s.task.load() != nullptr) {
        svc.log.logf(core::LogLevel::Warn,
                     "Audio task is already running, ignoring start request");
        return;
    }

    s.running.store(true);
    s.task.store(svc.tasks.start("audio_proc_task", kTaskStack, kTaskPriority, kTaskCore,
                                 audio_task, nullptr));

    if (s.task.load() == nullptr) {
        svc.log.logf(core::LogLevel::Error, "Failed to create audio task!");
        s.running.store(false);
        return;
    }

    svc.log.logf(core::LogLevel::Info, "Audio task successfully initialized and started");
    Network::send(protocol::IoT::client::encodeRecordAck(
        protocol::IoT::RecordCommand::START, svc.clock.nowUnixMs()));
}

void stop() {
    if (services() == nullptr) {
        return;
    }
    core::IAppServices& svc = *services();
    auto& s = state();

    if (s.task.load() == nullptr) {
        svc.log.logf(core::LogLevel::Warn,
                     "Audio task already stopped or not initialized");
        return;
    }

    svc.log.logf(core::LogLevel::Info, "Stopping audio task...");
    s.running.store(false);

    int timeout_ms = kStopTimeoutMs;
    while (s.task.load() != nullptr && timeout_ms > 0) {
        svc.tasks.delayMs(kLoopDelayMs);
        timeout_ms -= kLoopDelayMs;
    }

    if (s.task.load() != nullptr) {
        svc.log.logf(core::LogLevel::Warn,
                     "Force deleting audio task after timeout");
        svc.tasks.stop(s.task.exchange(nullptr));
    }

    svc.log.logf(core::LogLevel::Info, "Audio task completely stopped and deleted");
    Network::send(protocol::IoT::client::encodeRecordAck(
        protocol::IoT::RecordCommand::STOP, svc.clock.nowUnixMs()));
}

bool is_running() {
    return state().task.load() != nullptr;
}

} // namespace AudioService
