#pragma once

// Portable audio frame math shared by every audio backend.

#include <cstddef>
#include <cstdint>

namespace core::audio {

constexpr std::size_t samplesPerFrame(uint32_t sample_rate, uint32_t frame_ms) {
    return static_cast<std::size_t>(sample_rate) * frame_ms / 1000u;
}

constexpr std::size_t bytesPerFrame(uint32_t sample_rate, uint32_t frame_ms,
                                    uint32_t channels, uint32_t bits_per_sample) {
    return samplesPerFrame(sample_rate, frame_ms) * channels * (bits_per_sample / 8u);
}

} // namespace core::audio
