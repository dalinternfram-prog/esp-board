#include "Opus.hpp"
#include "general/log.hpp"  // Portable logging: general/ must not use ESP-IDF.

#include <cstring>

static const char* TAG = "Opus";
static constexpr int MAX_PACKET_SIZE = 1000;

Opus::Opus()
    : encoder_(nullptr),
      channelCount_(0),
      bitsPerChannel_(0),
      frameSize_(0) {}

Opus::~Opus() {
    release();
}

bool Opus::init(int sampleRate, int channelCount, int bitsPerChannel,
                int targetFrameDurationMs) {
    release(); // Releases any previous encoder.

    const bool validSampleRate =
        sampleRate == 8000 || sampleRate == 12000 ||
        sampleRate == 16000 || sampleRate == 24000 ||
        sampleRate == 48000;
    const bool validChannels = channelCount == 1 || channelCount == 2;
    const bool validBitDepth = bitsPerChannel == 16 || bitsPerChannel == 32;
    const bool validDuration =
        targetFrameDurationMs == 5 || targetFrameDurationMs == 10 ||
        targetFrameDurationMs == 20 || targetFrameDurationMs == 40 ||
        targetFrameDurationMs == 60;

    if (!validSampleRate || !validChannels || !validBitDepth || !validDuration) {
        LOG_E(TAG, "Invalid Opus configuration");
        return false;
    }

    frameSize_ = sampleRate * targetFrameDurationMs / 1000;

    int error = OPUS_OK;
    encoder_ = opus_encoder_create(sampleRate, channelCount, OPUS_APPLICATION_AUDIO, &error);

    if (error != OPUS_OK || encoder_ == nullptr) {
        LOG_E(TAG, "Encoder creation failed: %s", opus_strerror(error));
        encoder_ = nullptr;
        frameSize_ = 0;
        return false;
    }

    bitsPerChannel_ = bitsPerChannel;
    channelCount_ = channelCount;
    LOG_I(TAG, "Encoder initialized: %d Hz, %d channel(s), %d-bit PCM, frame size %d",
          sampleRate, channelCount, bitsPerChannel_, frameSize_);

    int complexity = 3;
    int err = opus_encoder_ctl(encoder_, OPUS_SET_COMPLEXITY(complexity));
    if (err != OPUS_OK) {
        LOG_E(TAG, "opus_encoder_ctl failed: %s", opus_strerror(err));
    }

    return true;
}

std::vector<unsigned char> Opus::encode(const void* pcmData) {
    if (encoder_ == nullptr || pcmData == nullptr) {
        LOG_E(TAG, "Encoder is uninitialized or PCM input is null");
        return {};
    }

    // Reuse the scratch packet buffer: no per-frame allocation.
    packet_scratch_.assign(MAX_PACKET_SIZE, 0);

    int bytesWritten = 0;

    if (bitsPerChannel_ == 16) {
        bytesWritten = opus_encode(
            encoder_,
            static_cast<const opus_int16*>(pcmData),
            frameSize_,
            packet_scratch_.data(),
            static_cast<opus_int32>(packet_scratch_.size()));
    } else if (bitsPerChannel_ == 32) {
        // Interpret input as signed 32-bit PCM. memcpy avoids strict-aliasing
        // UB on the caller's buffer.
        const auto* raw = static_cast<const uint8_t*>(pcmData);
        const std::size_t sampleCount =
            static_cast<std::size_t>(frameSize_) *
            static_cast<std::size_t>(channelCount_);

        float_scratch_.resize(sampleCount);
        for (std::size_t i = 0; i < sampleCount; ++i) {
            int32_t sample = 0;
            std::memcpy(&sample, raw + i * sizeof(int32_t), sizeof(int32_t));
            float_scratch_[i] = static_cast<float>(sample) / 2147483648.0f;
        }

        bytesWritten = opus_encode_float(
            encoder_,
            float_scratch_.data(),
            frameSize_,
            packet_scratch_.data(),
            static_cast<opus_int32>(packet_scratch_.size()));
    } else {
        LOG_E(TAG, "Unsupported PCM bit depth: %d", bitsPerChannel_);
        return {};
    }

    if (bytesWritten < 0) {
        LOG_E(TAG, "Encoding failed: %s", opus_strerror(bytesWritten));
        return {};
    }

    packet_scratch_.resize(static_cast<std::size_t>(bytesWritten));
    return packet_scratch_;
}

void Opus::release() {
    if (encoder_ != nullptr) {
        opus_encoder_destroy(encoder_);
        encoder_ = nullptr;
    }

    channelCount_ = 0;
    bitsPerChannel_ = 0;
    frameSize_ = 0;
    float_scratch_.clear();
    packet_scratch_.clear();
}
