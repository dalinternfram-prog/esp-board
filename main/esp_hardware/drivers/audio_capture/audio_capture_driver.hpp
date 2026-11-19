 #pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "esp_err.h"
#include "interface.hpp"

namespace Hardware {

class AudioCaptureDriver {
public:
    explicit AudioCaptureDriver(const core::AudioConfig& config);
    ~AudioCaptureDriver();

    AudioCaptureDriver(const AudioCaptureDriver&) = delete;
    AudioCaptureDriver& operator=(const AudioCaptureDriver&) = delete;

    AudioCaptureDriver(AudioCaptureDriver&& other) noexcept;
    AudioCaptureDriver& operator=(AudioCaptureDriver&& other) noexcept;

    [[nodiscard]] esp_err_t readSamples();
    // Copies the raw I2S frame bytes (slot layout untouched).
    [[nodiscard]] bool copyFrame(std::vector<uint8_t>& out) const;
    [[nodiscard]] std::size_t channelCount() const noexcept { return channel_count_; }
    [[nodiscard]] std::size_t chunkSize() const noexcept { return chunk_size_; }

    // True when the PSRAM DMA buffer was allocated successfully.
    [[nodiscard]] bool isReady() const noexcept { return raw_dma_buffer_ != nullptr; }

private:
    void freeBuffer() noexcept;

    core::AudioConfig config_{};
    std::size_t channel_count_{0};
    std::size_t chunk_size_{0};
    int16_t* raw_dma_buffer_{nullptr};
};

} // namespace Hardware
