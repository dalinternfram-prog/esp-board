// AudioService: capture -> encode -> send, composed from core ports.
// Platform access (I2S, Opus, tasks, clock, log) comes from app::context().

#include <atomic>
#include <cstdint>

#include "app/audio_service.hpp"
#include "app_context.hpp"

#include "audio_protocol.hpp"      // protocol::IoT::client::encodeAudio
#include "core/audio/AudioPipeline.hpp"
#include "network/net_manager.hpp"
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

void audio_task(void* /*arg*/) {
    auto& ctx = app::context();
    ctx.logger.logf(core::LogLevel::Info, "Audio task created & initializing");

    if (!ctx.audio_in.open() || !ctx.audio_encoder.init()) {
        ctx.logger.logf(core::LogLevel::Error, "Audio pipeline init failed");
        state().task.store(nullptr);
        ctx.tasks.exitCurrentTask();
        return;
    }

    core::AudioPipeline pipeline(
        ctx.audio_in, ctx.audio_encoder, ctx.clock, ctx.logger,
        [](const core::Bytes& compressed, uint32_t timestamp_ms) {
            Network::send(protocol::IoT::client::encodeAudio(compressed, timestamp_ms));
        });

    while (state().running.load()) {
        pipeline.step();
        ctx.tasks.delayMs(kLoopDelayMs);
    }

    ctx.audio_in.close();
    ctx.logger.logf(core::LogLevel::Info, "Exiting audio loop: releasing drivers");
    state().task.store(nullptr);
    ctx.tasks.exitCurrentTask();
}

} // namespace

void start() {
    auto& ctx = app::context();
    auto& s = state();

    if (s.task.load() != nullptr) {
        ctx.logger.logf(core::LogLevel::Warn,
                        "Audio task is already running, ignoring start request");
        return;
    }

    s.running.store(true);
    s.task.store(ctx.tasks.start("audio_proc_task", kTaskStack, kTaskPriority, kTaskCore,
                                 audio_task, nullptr));

    if (s.task.load() == nullptr) {
        ctx.logger.logf(core::LogLevel::Error, "Failed to create audio task!");
        s.running.store(false);
        return;
    }

    ctx.logger.logf(core::LogLevel::Info, "Audio task successfully initialized and started");
    Network::send(protocol::IoT::client::encodeRecordAck(
        protocol::IoT::RecordCommand::START, ctx.clock.nowUnixMs()));
}

void stop() {
    auto& ctx = app::context();
    auto& s = state();

    if (s.task.load() == nullptr) {
        ctx.logger.logf(core::LogLevel::Warn,
                        "Audio task already stopped or not initialized");
        return;
    }

    ctx.logger.logf(core::LogLevel::Info, "Stopping audio task...");
    s.running.store(false);

    int timeout_ms = kStopTimeoutMs;
    while (s.task.load() != nullptr && timeout_ms > 0) {
        ctx.tasks.delayMs(kLoopDelayMs);
        timeout_ms -= kLoopDelayMs;
    }

    if (s.task.load() != nullptr) {
        ctx.logger.logf(core::LogLevel::Warn,
                        "Force deleting audio task after timeout");
        ctx.tasks.stop(s.task.exchange(nullptr));
    }

    ctx.logger.logf(core::LogLevel::Info, "Audio task completely stopped and deleted");
    Network::send(protocol::IoT::client::encodeRecordAck(
        protocol::IoT::RecordCommand::STOP, ctx.clock.nowUnixMs()));
}

bool is_running() {
    return state().task.load() != nullptr;
}

} // namespace AudioService
