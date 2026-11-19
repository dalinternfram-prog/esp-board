#include "core/audio/AudioPipeline.hpp"

#include <cstdio>
#include <utility>

namespace core {

AudioPipeline::AudioPipeline(IAudioSource& source, IAudioEncoder& encoder, IClock& clock,
                             ILogger& logger, Sender sender)
    : source_(source),
      encoder_(encoder),
      clock_(clock),
      logger_(logger),
      sender_(std::move(sender)) {}

void AudioPipeline::reset() {
    frame_count_ = 0;
}

bool AudioPipeline::step() {
    if (!source_.readFrame(pcm_)) {
        return false;
    }

    Bytes compressed;
    if (!encoder_.encode(pcm_, compressed)) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "Encode failed (frame %u)",
                      static_cast<unsigned>(frame_count_));
        logger_.log(LogLevel::Error, buf);
        return false;
    }

    // Stream-relative time from frame progress: the first packet is exactly 0
    // (protocol: "0 = starts a new recording"), immune to scheduling jitter.
    const uint32_t timestamp_ms = frame_count_ * encoder_.frameDurationMs();
    if (sender_) {
        sender_(compressed, timestamp_ms);
    }
    ++frame_count_;
    return true;
}

} // namespace core
