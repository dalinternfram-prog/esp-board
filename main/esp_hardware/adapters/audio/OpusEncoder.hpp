#pragma once

// Adapter: Opus encoder bound to the project's audio arguments.

#include "interface.hpp"

#include <vector>

#include "Opus.hpp"

namespace adapters {

class OpusEncoder : public core::IAudioEncoder {
public:
    explicit OpusEncoder(const core::AudioConfig& config) : config_(config) {}

    bool init() override {
        return encoder_.init(static_cast<int>(config_.sample_rate),
                             static_cast<int>(config_.channel_count),
                             static_cast<int>(config_.bits_per_chan),
                             static_cast<int>(config_.target_frame_duration_ms));
    }

    bool encode(const std::vector<int16_t>& pcm, core::Bytes& out) override {
        auto encoded = encoder_.encode(pcm.data());
        if (encoded.empty()) {
            return false;
        }
        out.assign(encoded.begin(), encoded.end());
        return true;
    }

    uint32_t frameDurationMs() const override {
        return config_.target_frame_duration_ms;
    }

    uint32_t sampleRate() const override { return config_.sample_rate; }
    uint32_t channelCount() const override { return config_.channel_count; }

private:
    core::AudioConfig config_;
    Opus encoder_;
};

} // namespace adapters
