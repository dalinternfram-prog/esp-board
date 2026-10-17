#include "audio_capture_driver.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "bsp_board.h" // Hardware BSP layer
#include "driver/i2s_std.h"
#include "portmacro.h"
#include "macroPhone/arguments.hpp"
//* this class can now only handle one i2s line,which each has its own rx_handle;
//*
static const char* TAG = "AudioCaptureDriver";

// Default standard AFE audio frame size for 16kHz sampling (e.g., 32ms frame = 512 samples)
// static constexpr std::size_t kDefaultChunkSize = 512;

namespace Hardware {

AudioCaptureDriver::AudioCaptureDriver() {
    // 1. Discover hardware configuration directly from BSP
   // channel_count_ = static_cast<std::size_t>(esp_get_feed_channel());

 


// i2s_chan_info_t chan_info{};
// if (i2s_channel_get_info(bsp_i2s_get_rx_handle(), &chan_info) == ESP_OK) {
//     if (chan_info.mode == I2S_COMM_MODE_STD && chan_info.mode_cfg != nullptr) {
//         auto* std_cfg = static_cast<const i2s_std_config_t*>(chan_info.mode_cfg);
        
//         // MONO slot mode = 1 channel, STEREO slot mode = 2 channels
//         channel_count_ = (std_cfg->slot_cfg.slot_mode == I2S_SLOT_MODE_MONO) ? 1 : 2;
//     } else {
        
//        ESP_LOGW("++++++++++", "----  ==  --- default channel count = 2");
//         channel_count_ = 2; // Default hardware fallback for standard stereo
        
//     }
// } else {
 
//     channel_count_ = 2;
//        ESP_LOGW("++++++++++", "default channel count = 2");
// }

channel_count_=macroPhone::argument::channel_count;
chunk_size_=macroPhone::argument::kDefaultChunkSize;




    //chunk_size_ = kDefaultChunkSize;

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
    
    // Allocate and copy data from raw_dma_buffer_ into a new std::vector
    return std::vector<int16_t>(raw_dma_buffer_, raw_dma_buffer_ + total_samples);
}




} // namespace Hardware