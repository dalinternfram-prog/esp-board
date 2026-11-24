#include "audio_capture_driver.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "bsp_board.h"
#include "driver/i2s_std.h"
#include "portmacro.h"

#include <cstring>

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

    // The I2S capture buffer must live in internal RAM on this board. PSRAM
    // shares the cache/MMU path with the flash/XIP mappings; keeping the audio
    // buffer in PSRAM corrupts the heap on the first frame. Fall back to PSRAM
    // only if internal RAM is genuinely unavailable.
    raw_dma_buffer_ = static_cast<int16_t*>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (raw_dma_buffer_ == nullptr) {
        ESP_LOGW(TAG, "Internal audio buffer unavailable; trying PSRAM");
        raw_dma_buffer_ = static_cast<int16_t*>(
            heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }

    if (raw_dma_buffer_ == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes for audio buffer", buffer_bytes);
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

    // Use the BSP/vendor capture path instead of a raw i2s_channel_read().
    // The record codec device owns the I2S RX channel and its DMA/queue state;
    // reading the channel directly bypasses that ownership and corrupts the
    // I2S driver's FreeRTOS queue lists (xQueueReceive/vListInsert panic).
    const esp_err_t err = esp_get_feed_data(
        /*is_get_raw_channel=*/true, raw_dma_buffer_,
        static_cast<int>(buffer_bytes));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_get_feed_data failed: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool AudioCaptureDriver::copyFrame(std::vector<uint8_t>& out) const {
    if (raw_dma_buffer_ == nullptr) {
        return false;
    }

    const std::size_t frame_bytes = chunk_size_ * channel_count_ *
                                    (config_.bits_per_chan / 8u);
    const auto* bytes = reinterpret_cast<const uint8_t*>(raw_dma_buffer_);

    if (config_.bits_per_chan == config_.pcm_bits_per_chan) {
        out.assign(bytes, bytes + frame_bytes);
        return true;
    }

    // ES7210 packs four 16-bit mics into two 32-bit I2S slots. The signal
    // mics are the HIGH halves (mic1 in slot 0, mic3 in slot 1); the low
    // halves are the reference/noise pair that the vendor's 3-channel
    // esp_get_feed_data path drops as well. Emit them as PCM16 stereo instead
    // of reinterpreting a packed pair as one 32-bit sample (which scaled the
    // high mic correctly but threw the low mic away as 2^-31 noise).
    if (config_.bits_per_chan == 32 && config_.pcm_bits_per_chan == 16 &&
        channel_count_ == 2) {
        const std::size_t words = frame_bytes / sizeof(uint32_t);
        out.resize(words * sizeof(int16_t));
        auto* dst = reinterpret_cast<int16_t*>(out.data());
        for (std::size_t i = 0; i < words; ++i) {
            uint32_t word = 0;
            std::memcpy(&word, bytes + i * sizeof(uint32_t), sizeof word);
            dst[i] = static_cast<int16_t>(static_cast<uint16_t>(word >> 16));
        }
        return true;
    }

    ESP_LOGE(TAG,
             "Unsupported PCM conversion: i2s=%u bits, pcm=%u bits, channels=%zu",
             static_cast<unsigned>(config_.bits_per_chan),
             static_cast<unsigned>(config_.pcm_bits_per_chan),
             channel_count_);
    return false;
}

} // namespace Hardware
