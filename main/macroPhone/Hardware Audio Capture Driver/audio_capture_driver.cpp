 
#include "audio_capture_driver.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "bsp_board.h" // Hardware BSP layer
#include "driver/i2s_std.h"
#include "portmacro.h"
#include "macroPhone/arguments.hpp" // Directly access project arguments

static const char* TAG = "AudioCaptureDriver";

namespace Hardware {

AudioCaptureDriver::AudioCaptureDriver() {
    // Directly pull from macroPhone::argument:: without intermediate wrappers
    channel_count_ = macroPhone::argument::channel_count;
    chunk_size_ = (macroPhone::argument::sample_rate * macroPhone::argument::target_frame_duration_ms) / 1000;

    const std::size_t total_samples = chunk_size_ * channel_count_;
    const std::size_t buffer_bytes = total_samples * (macroPhone::argument::bits_per_chan / 8);

    // Hide PSRAM memory management entirely inside Level 1
    raw_dma_buffer_ = static_cast<int16_t*>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
    );

    if (raw_dma_buffer_ == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes in PSRAM for audio buffer", buffer_bytes);
    } else {
        ESP_LOGI(TAG, "Audio Hardware Capture initialized (%zu channels, %zu samples/channel, %lu ms duration)", 
                 channel_count_, chunk_size_, macroPhone::argument::target_frame_duration_ms);
    }
}

AudioCaptureDriver::~AudioCaptureDriver() {
    freeBuffer();
}

AudioCaptureDriver::AudioCaptureDriver(AudioCaptureDriver&& other) noexcept
    : channel_count_(other.channel_count_), 
      chunk_size_(other.chunk_size_), 
      raw_dma_buffer_(other.raw_dma_buffer_) {
    other.raw_dma_buffer_ = nullptr;
    other.channel_count_ = 0;
    other.chunk_size_ = 0;
}

AudioCaptureDriver& AudioCaptureDriver::operator=(AudioCaptureDriver&& other) noexcept {
    if (this != &other) {
        freeBuffer();
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
    const std::size_t buffer_bytes = chunk_size_ * channel_count_ * (macroPhone::argument::bits_per_chan / 8);
    size_t bytes_read = 0;

    // Use the handle returned when setting up i2s_new_rx_channel()
    esp_err_t err = i2s_channel_read(bsp_i2s_get_rx_handle(), this->raw_dma_buffer_, buffer_bytes, &bytes_read, 1000);

    if (err != ESP_OK || bytes_read == 0) {
        ESP_LOGW(TAG, "Failed to read PCM data directly from I2S");
        return ESP_FAIL;
    }

    return ESP_OK;
}

std::vector<int16_t> AudioCaptureDriver::getFrameBuffer() const {
    if (raw_dma_buffer_ == nullptr) {
        return {};
    }

    const std::size_t total_samples = chunk_size_ * channel_count_;


    // Print the first 6 raw samples coming straight out of the I2S DMA buffer
    // ESP_LOGI("ESP32_CAPTURE", "RAW STREAM -> [0]:%d, [1]:%d, [2]:%d, [3]:%d, [4]:%d, [5]:%d",
    //          raw_dma_buffer_[0], raw_dma_buffer_[1],
    //          raw_dma_buffer_[2], raw_dma_buffer_[3],
    //          raw_dma_buffer_[4], raw_dma_buffer_[5]);
    // --------------------------------


    // Allocate and copy data from raw_dma_buffer_ into a new std::vector
    return std::vector<int16_t>(raw_dma_buffer_, raw_dma_buffer_ + total_samples);
}

} // namespace Hardware