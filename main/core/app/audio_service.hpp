#pragma once

// Capture-session service: IAudioSource -> AudioPipeline -> INetwork.
// A plain class owned by the composition root; no global state.

#include <atomic>
#include <cstdint>
#include <mutex>

#include "interface.hpp"

namespace core::app {

class AudioService final : public IAudioService {
public:
    AudioService(IAudioSource& input, IAudioEncoder& encoder, IClock& clock,
                 ITaskRunner& tasks, ILogger& logger, INetwork& network);
    ~AudioService() override;

    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;

    // Starts capture/encode/send. The START ack is sent only after the
    // hardware/codec are actually open (ok=false on failure).
    void start(uint32_t request_id = 0) override;

    // Stops the task and releases the audio hardware. When ack is false the
    // STOP is silent (used for reconnect cleanup, where no session exists).
    void stop(uint32_t request_id = 0, bool ack = true) override;

    bool isRunning() const override;

private:
    enum class RunState { Stopped, Running, Stopping };

    static void taskEntry(void* arg);
    void taskLoop();
    void clearTaskHandle();

    IAudioSource&  input_;
    IAudioEncoder& encoder_;
    IClock&        clock_;
    ITaskRunner&   tasks_;
    ILogger&       logger_;
    INetwork&      network_;

    std::mutex mutex_;   // task-handle publication vs. the task clearing it
    std::atomic<ITaskRunner::TaskId> task_{nullptr};
    std::atomic<bool> running_{false};
    std::atomic<RunState> run_state_{RunState::Stopped};
    std::atomic<uint32_t> request_id_{0};   // RecordControl.requestId
};

} // namespace core::app
