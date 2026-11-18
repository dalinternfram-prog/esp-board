#include "audio_capture_driver.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "bsp_board.h"
#include "driver/i2s_std.h"
#include "portmacro.h"

#include "general/audio/frame_math.hpp"

static const char* TAG = "AudioCaptureDriver";

namespace Hardware {

AudioCaptureDriver::AudioCaptureDriver(const core::AudioConfig& config)
    : config_(config) {
    channel_count_ = config_.channel_count;
    chunk_size_ = general::audio::samplesPerFrame(config_.sample_rate,
                                                  config_.target_frame_duration_ms);

    const std::size_t buffer_bytes = general::audio::bytesPerFrame(
        config_.sample_rate, config_.target_frame_duration_ms,
        config_.channel_count, config_.bits_per_chan);

    raw_dma_buffer_ = static_cast<int16_t*>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (raw_dma_buffer_ == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes in PSRAM for audio buffer", buffer_bytes);
    } else {
        ESP_LOGI(TAG, "Audio Hardware Capture initialized (%zu channels, %zu samples/channel, %lu ms duration)",
                 channel_count_, chunk_size_,
                 static_cast<unsigned long>(config_.target_frame_duration_ms));
    }
}

AudioCaptureDriver::~AudioCaptureDriver() {
    freeBuffer();
}

AudioCaptureDriver::AudioCaptureDriver(AudioCaptureDriver&& other) noexcept
    : config_(other.config_),
      channel_count_(other.channel_count_),
      chunk_size_(other.chunk_size_),
      raw_dma_buffer_(other.raw_dma_buffer_) {
    other.raw_dma_buffer_ = nullptr;
    other.channel_count_ = 0;
    other.chunk_size_ = 0;
}

AudioCaptureDriver& AudioCaptureDriver::operator=(AudioCaptureDriver&& other) noexcept {
    if (this != &other) {
        freeBuffer();
        config_ = other.config_;
        channel_count_ = other.channel_count_;
        chunk_size_ = other.chunk_size_;
        raw_dma_buffer_ = other.raw_dma_buffer_;
        other.raw_dma_buffer_ = nullptr;
        other.channel_count_ = 0;
        other.chunk_size_ = 0;
    }
    return *this;
}

void AudioCaptureDriver::freeBuffer() noexcept {
    if (raw_dma_buffer_ != nullptr) {
        heap_caps_free(raw_dma_buffer_);
        raw_dma_buffer_ = nullptr;
    }
}

esp_err_t AudioCaptureDriver::readSamples() {
    if (raw_dma_buffer_ == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    const std::size_t buffer_bytes = general::audio::bytesPerFrame(
        config_.sample_rate, config_.target_frame_duration_ms,
        config_.channel_count, config_.bits_per_chan);
    size_t bytes_read = 0;

    esp_err_t err = i2s_channel_read(bsp_i2s_get_rx_handle(), raw_dma_buffer_,
                                     buffer_bytes, &bytes_read,
                                     /*timeout_ms=*/100);  // < AudioService stop window
    if (err != ESP_OK || bytes_read == 0) {
        ESP_LOGW(TAG, "Failed to read PCM data directly from I2S");
        return ESP_FAIL;
    }

    if (bytes_read != buffer_bytes) {
        ESP_LOGW(TAG, "Short I2S read: %u / %u bytes",
                 static_cast<unsigned>(bytes_read),
                 static_cast<unsigned>(buffer_bytes));
        return ESP_FAIL;
    }

    return ESP_OK;
}

std::vector<int16_t> AudioCaptureDriver::getFrameBuffer() const {
    if (raw_dma_buffer_ == nullptr) {
        return {};
    }

    const std::size_t total_samples = chunk_size_ * channel_count_;

    int factor = 1;
    if (config_.bits_per_chan == 32) {
        factor = 2;
    }

    return std::vector<int16_t>(raw_dma_buffer_, raw_dma_buffer_ + total_samples * factor);
}

} // namespace Hardware
