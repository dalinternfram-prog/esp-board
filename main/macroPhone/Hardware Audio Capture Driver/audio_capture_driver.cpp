#include "audio_capture_driver.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "bsp_board.h" // Hardware BSP layer

static const char* TAG = "AudioCaptureDriver";

// Default standard AFE audio frame size for 16kHz sampling (e.g., 32ms frame = 512 samples)
static constexpr std::size_t kDefaultChunkSize = 512;

namespace Hardware {

AudioCaptureDriver::AudioCaptureDriver() {
    // 1. Discover hardware configuration directly from BSP
    channel_count_ = static_cast<std::size_t>(esp_get_feed_channel());
    chunk_size_ = kDefaultChunkSize;

    const std::size_t total_samples = chunk_size_ * channel_count_;
    const std::size_t buffer_bytes = total_samples * sizeof(int16_t);

    // 2. Hide PSRAM memory management entirely inside Level 1
    raw_dma_buffer_ = static_cast<int16_t*>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
    );

    if (raw_dma_buffer_ == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes in PSRAM for audio buffer", buffer_bytes);
    } else {
        ESP_LOGI(TAG, "Audio Hardware Capture initialized (%zu channels, %zu samples/chunk)", 
                 channel_count_, chunk_size_);
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

    const std::size_t buffer_bytes = chunk_size_ * channel_count_ * sizeof(int16_t);

    // Fetch hardware audio directly into internal PSRAM buffer
    int bytes_read = esp_get_feed_data(true, raw_dma_buffer_, static_cast<int>(buffer_bytes));

    if (bytes_read <= 0) {
        ESP_LOGW(TAG, "Failed to fetch audio stream from hardware");
        return ESP_FAIL;
    }

    return ESP_OK;
}

std::span<const int16_t> AudioCaptureDriver::getFrameBuffer() const noexcept {
    if (raw_dma_buffer_ == nullptr) {
        return {};
    }
    return std::span<const int16_t>(raw_dma_buffer_, chunk_size_ * channel_count_);
}

} // namespace Hardware