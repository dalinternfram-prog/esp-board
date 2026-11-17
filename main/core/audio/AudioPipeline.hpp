#pragma once

// Domain core: capture -> encode -> emit. Pure C++ over the audio ports.
// The application decides what to do with the compressed frame (e.g. wrap it
// in the IoT protocol and queue it for the session transport).

#include "interface.hpp"

#include <functional>
#include <vector>

namespace core {

class AudioPipeline {
public:
    // Called once per encoded frame. timestampMs is stream-relative (0-based).
    using Sender = std::function<void(const Bytes& compressed, uint32_t timestampMs)>;

    AudioPipeline(IAudioSource& source, IAudioEncoder& encoder, IClock& clock,
                  ILogger& logger, Sender sender);

    // One capture -> encode -> emit cycle. Returns false when no frame was sent.
    bool step();

    // Restart the stream-relative timestamp (e.g. on a new recording).
    void reset();

private:
    IAudioSource&  source_;
    IAudioEncoder& encoder_;
    IClock&        clock_;
    ILogger&       logger_;
    Sender         sender_;

    std::vector<int16_t> pcm_;
    uint32_t frame_count_ = 0;
    uint32_t session_start_ms_ = 0;
};

} // namespace core
