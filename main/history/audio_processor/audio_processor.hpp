// #pragma once

// #include "audio_capture_driver.hpp"

// #include <span>
// #include "esp_err.h"
// #include "esp_afe_sr_iface.h" // Includes official typedefs for esp_afe_sr_iface_t and esp_afe_sr_data_t
// #include "esp_afe_sr_models.h"
// #include<memory>
// struct esp_afe_sr_data_s; 

// namespace AudioProcessing {

// /**
//  * @brief Clean DSP output frame containing filtered single-channel audio.
//  */
// struct ProcessedFrame {
//     std::span<const int16_t> pcm_data;  ///< Clean, noise-suppressed audio
//     std::size_t trigger_channel_id{0};   ///< Primary microphone channel index
// };

// /**
//  * @brief Level 2 Engine: Strictly responsible for DSP signal conditioning.
//  */
// class AudioProcessor {




// public:
//     AudioProcessor();
//     ~AudioProcessor();

//     AudioProcessor(const AudioProcessor&) = delete;
//     AudioProcessor& operator=(const AudioProcessor&) = delete;
//     AudioProcessor(AudioProcessor&&) = delete;
//     AudioProcessor& operator=(AudioProcessor&&) = delete;

//     /**
//      * @brief Reads raw hardware audio, applies DSP filters, and returns clean PCM data.
//      */
//     [[nodiscard]] esp_err_t processNextFrame(ProcessedFrame& out_frame);


// private:
//     std::unique_ptr<Hardware::AudioCaptureDriver> capture_driver_;
//       esp_afe_sr_iface_t* afe_handle_ ;
//     esp_afe_sr_data_t* afe_data_ ;
//     srmodel_list_t *models;
// };
// } // namespace AudioProcessing