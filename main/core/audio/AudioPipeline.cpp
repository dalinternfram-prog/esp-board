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
      sender_(std::move(sender)) {
    session_start_ms_ = clock_.nowMs();
}

void AudioPipeline::reset() {
    frame_count_ = 0;
    session_start_ms_ = clock_.nowMs();
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

    const uint32_t timestamp_ms = clock_.nowMs() - session_start_ms_;
    if (sender_) {
        sender_(compressed, timestamp_ms);
    }
    ++frame_count_;
    return true;
}

} // namespace core
