
// // #include "esp_afe_sr_iface.h"
// // #include "esp_afe_sr_models.h"
// // #include "audio_processor.hpp"

// // #include "esp_log.h" 
// // static const char* TAG = "AudioProcessor";

// // namespace AudioProcessing {

// // AudioProcessor::AudioProcessor()
// //     : capture_driver_(std::make_unique<Hardware::AudioCaptureDriver>()) {

// //     afe_config_t afe_config = AFE_CONFIG_DEFAULT();
// //     // Enable DSP features only (Voice communication / AEC / NS)
// //     afe_config.wakenet_init = true; 
// //     afe_config.voice_communication_init = false;

// //     afe_handle_ = &ESP_AFE_SR_HANDLE;
// //     afe_data_ = afe_handle_->create_from_config(&afe_config);

// //     configASSERT(afe_data_ != nullptr);
// //     ESP_LOGI(TAG, "Audio DSP Engine (Level 2) initialized successfully");
// // }

// // AudioProcessor::~AudioProcessor() {
// //     if (afe_handle_ != nullptr && afe_data_ != nullptr) {
// //         afe_handle_->destroy(afe_data_);
// //         afe_data_ = nullptr;
// //     }
// // }

// // esp_err_t AudioProcessor::processNextFrame(ProcessedFrame& out_frame) {
// //     if (!capture_driver_) {
// //         return ESP_ERR_INVALID_STATE;
// //     }

// //     esp_err_t err = capture_driver_->readSamples();
// //     if (err != ESP_OK) {
// //         return err;
// //     }

// //     auto raw_buffer = capture_driver_->getFrameBuffer();
// //     if (raw_buffer.empty()) {
// //         return ESP_FAIL;
// //     }

// //     // 1. Feed raw audio into DSP engine
// //     afe_handle_->feed(afe_data_, raw_buffer.data());

// //     // 2. Fetch clean single-channel audio
// //     afe_fetch_result_t* fetch_result = afe_handle_->fetch(afe_data_);
// //     if (fetch_result == nullptr || fetch_result->ret_value == ESP_FAIL) {
// //         return ESP_FAIL;
// //     }

// //     const std::size_t fetch_samples = fetch_result->data_size / sizeof(int16_t);

// //     // 3. Export clean audio frame (Zero ML logic)
// //     out_frame.pcm_data = std::span<const int16_t>(
// //         reinterpret_cast<const int16_t*>(fetch_result->data),
// //         fetch_samples
// //     );
// //     out_frame.trigger_channel_id = static_cast<std::size_t>(fetch_result->trigger_channel_id);

// //     return ESP_OK;
// // }

// // } // namespace AudioProcessing

// #include "esp_afe_sr_iface.h"
// #include "esp_afe_sr_models.h"
// #include "audio_processor.hpp"

// #include "bsp_board.h"
// #include "esp_wn_iface.h"
// #include "esp_wn_models.h"
// #include "esp_afe_sr_iface.h"
// #include "esp_afe_sr_models.h"
// #include "esp_mn_iface.h"
// #include "esp_mn_models.h"
// #include "model_path.h"
// #include "esp_process_sdkconfig.h"

// #include "bsp_board.h"
// #include "esp_log.h"
// #include "esp_task_wdt.h" 
// #include "esp_log.h" 
// static const char* TAG = "AudioProcessor";

// namespace AudioProcessing {
// int wakeup_flag = 0; 
// static volatile int task_flag = 0;
// srmodel_list_t *models = NULL;

 
// AudioProcessor::AudioProcessor()
//     : capture_driver_(std::make_unique<Hardware::AudioCaptureDriver>()) {

//     // 1. Use modern initialization macro with WakeNet (Speech Recognition mode)
//     // afe_config_t afe_config = AFE_CONFIG_DEFAULT();

//     // // 2. Configure WakeNet & Voice Communication features via the updated struct layout
//     // afe_config.wakenet_model_name = nullptr; // Uses default model from partition/flash
//     // afe_config.wakenet_mode = DET_MODE_2CH_95;

//     // // 3. Obtain the handle using the modern interface getter
//     // afe_handle_ = (esp_afe_sr_iface_t*)&esp_afe_sr_get_interface;
    
//     // // 4. Create AFE instance
//     // afe_data_ = afe_handle_->create_from_config(&afe_config);




// //     this->models = esp_srmodel_init("model"); // partition label defined in partitions.csv



// //     afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
// //     afe_config->ns_init = false;
// //     afe_config->vad_init = false;
// //   this-> afe_handle_ = esp_afe_handle_from_config(afe_config);
 

// //     this->afe_data_ =    this-> afe_handle_ ->create_from_config(afe_config);

// //     afe_config_free(afe_config);

// //     configASSERT(this->afe_data_ != nullptr);
// //     ESP_LOGI(TAG, "Audio DSP Engine (Level 2) initialized successfully");



//     models = esp_srmodel_init("model"); // partition label defined in partitions.csv


//     afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
//     afe_config->ns_init = false;
//     afe_config->vad_init = false;
//    this->afe_handle_   = const_cast<esp_afe_sr_iface_t*>(esp_afe_handle_from_config(afe_config));
//    this-> afe_data_ =    this->afe_handle_ ->create_from_config(afe_config);
//     afe_config_free(afe_config);



// }

// AudioProcessor::~AudioProcessor() {
//     if (   this->afe_handle_   != nullptr &&    this-> afe_data_  != nullptr) {
//       this->afe_handle_ ->destroy(   this-> afe_data_ );
//         this-> afe_data_  = nullptr;
//     }
// }

// esp_err_t AudioProcessor::processNextFrame(ProcessedFrame& out_frame) {
//     if (!capture_driver_) {
//         return ESP_ERR_INVALID_STATE;
//     }

//     esp_err_t err = capture_driver_->readSamples();
//     if (err != ESP_OK) {
//         return err;
//     }

//     auto raw_buffer = capture_driver_->getFrameBuffer();
//     if (raw_buffer.empty()) {
//         return ESP_FAIL;
//     }

//     // 1. Feed raw audio into DSP engine
//     afe_handle_->feed(afe_data_, raw_buffer.data());

//     // 2. Fetch processed audio frame
//     afe_fetch_result_t* fetch_result = afe_handle_->fetch(afe_data_);
//     if (fetch_result == nullptr || fetch_result->ret_value == ESP_FAIL) {
//         return ESP_FAIL;
//     }

//     const std::size_t fetch_samples = fetch_result->data_size / sizeof(int16_t);

//     // 3. Export clean audio frame
//     out_frame.pcm_data = std::span<const int16_t>(
//         reinterpret_cast<const int16_t*>(fetch_result->data),
//         fetch_samples
//     );
//     out_frame.trigger_channel_id = static_cast<std::size_t>(fetch_result->trigger_channel_id);

//     return ESP_OK;
// }

// } // namespace AudioProcessing