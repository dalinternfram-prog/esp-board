// #include "speech_engine.hpp"

// #include "esp_mn_iface.h"
// #include "esp_mn_models.h"
// #include "esp_srmodel_list.h"
// #include "esp_log.h"

// static const char* TAG = "SpeechEngine";

// namespace Speech {

// static constexpr auto kCommandTimeout = std::chrono::milliseconds(6000);

// SpeechEngine::SpeechEngine()
//     : audio_processor_(std::make_unique<AudioProcessing::AudioProcessor>()) {

//     // 1. Initialize models partition
//     models_ = esp_srmodel_init("model");
//     char* mn_name = esp_srmodel_filter(models_, ESP_MN_PREFIX, ESP_MN_CHINESE);

//     // 2. Load MultiNet interface and model data
//     multinet_ = esp_mn_handle_from_name(mn_name);
//     model_data_ = multinet_->create(mn_name, static_cast<int>(kCommandTimeout.count()));

//     configASSERT(model_data_ != nullptr);
//     ESP_LOGI(TAG, "Speech ML Engine (Level 3) initialized");
// }

// SpeechEngine::~SpeechEngine() {
//     if (multinet_ != nullptr && model_data_ != nullptr) {
//         multinet_->destroy(model_data_);
//         model_data_ = nullptr;
//     }
// }

// std::optional<SpeechEvent> SpeechEngine::pollEvent() {
//     if (!audio_processor_) {
//         return std::nullopt;
//     }

//     // 1. Read clean single-channel audio frame from Level 2 DSP Processor
//     AudioProcessing::ProcessedFrame frame;
//     if (audio_processor_->processNextFrame(frame) != ESP_OK || frame.pcm_data.empty()) {
//         return std::nullopt;
//     }

//     // 2. Process frame through ML state machine
//     switch (current_state_) {
//         case State::ListeningForWakeWord: {
//             // Check WakeNet status from frame or underlying pipeline
//             // If triggered:
//             // current_state_ = State::ListeningForCommand;
//             // return WakeWordEvent{};
//             break;
//         }

//         case State::ListeningForCommand: {
//             esp_mn_state_t mn_state = multinet_->detect(
//                 model_data_,
//                 const_cast<int16_t*>(frame.pcm_data.data())
//             );

//             if (mn_state == ESP_MN_STATE_DETECTED) {
//                 esp_mn_results_t* results = multinet_->get_results(model_data_);
//                 current_state_ = State::ListeningForWakeWord;

//                 return CommandEvent{static_cast<std::uint32_t>(results->command_id[0])};
//             } 
//             else if (mn_state == ESP_MN_STATE_TIMEOUT) {
//                 current_state_ = State::ListeningForWakeWord;

//                 return TimeoutEvent{};
//             }
//             break;
//         }
//     }

//     return std::nullopt;
// }

// } // namespace Speech