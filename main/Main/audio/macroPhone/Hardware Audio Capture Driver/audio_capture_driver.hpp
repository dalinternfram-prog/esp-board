 #pragma once

#include <span>
#include <cstdint>
#include <cstddef>
#include "esp_err.h"

#include<vector>
namespace Hardware {

/**
 * @brief Level 1 Hardware Driver: Completely encapsulates raw microphone 
 *        I2S hardware capture and PSRAM buffer lifecycle.
 * 
 * The caller requires zero knowledge of vendor AFE structs or DMA memory pointers.
 */
class AudioCaptureDriver {
public:
    /**
     * @brief Constructs driver by self-discovering hardware channel count 
     *        and allocating PSRAM memory internally.
     */
    AudioCaptureDriver();
    ~AudioCaptureDriver();

    AudioCaptureDriver(const AudioCaptureDriver&) = delete;
    AudioCaptureDriver& operator=(const AudioCaptureDriver&) = delete;

    AudioCaptureDriver(AudioCaptureDriver&& other) noexcept;
    AudioCaptureDriver& operator=(AudioCaptureDriver&& other) noexcept;

    /**
     * @brief Reads a frame of raw audio from I2S hardware into internal PSRAM.
     */
    [[nodiscard]] esp_err_t readSamples();

    /**
     * @brief Provides a read-only view of the latest captured audio frame.
     */
  //  [[nodiscard]] std::span<const int16_t> getFrameBuffer() const noexcept;
    [[nodiscard]]    std::vector<int16_t> getFrameBuffer() const;
    [[nodiscard]] std::size_t channelCount() const noexcept { return channel_count_; }
    [[nodiscard]] std::size_t chunkSize() const noexcept { return chunk_size_; }

   
private:
    void freeBuffer() noexcept;

    std::size_t channel_count_{0};
    std::size_t chunk_size_{0};
    int16_t* raw_dma_buffer_{nullptr};
};




} // namespace Hardware