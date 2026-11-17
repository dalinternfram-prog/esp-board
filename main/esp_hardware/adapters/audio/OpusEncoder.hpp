#pragma once

// Adapter: Opus encoder bound to the project's audio arguments.

#include "interface.hpp"

#include <vector>

#include "Opus.hpp"
#include "core/app/arguments.hpp"

namespace adapters {

class OpusEncoder : public core::IAudioEncoder {
public:
    bool init() override {
        return encoder_.init(macroPhone::argument::sample_rate,
                             macroPhone::argument::channel_count,
                             macroPhone::argument::bits_per_chan,
                             macroPhone::argument::target_frame_duration_ms);
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
        return static_cast<uint32_t>(macroPhone::argument::target_frame_duration_ms);
    }

private:
    Opus encoder_;
};

} // namespace adapters
