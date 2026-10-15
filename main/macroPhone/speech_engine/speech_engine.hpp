// #pragma once

// #include "audio_processor.hpp"

// #include <chrono>
// #include <cstdint>
// #include <memory>
// #include <optional>
// #include <variant>
// #include "esp_err.h"

// // Forward declaration of vendor C types to avoid leaking C headers into caller space
// struct srmodel_list_t;
// struct esp_mn_iface_s;
// struct model_iface_data_s;

// namespace Speech {

// struct WakeWordEvent {};

// struct CommandEvent {
//     std::uint32_t command_id{0};  ///< Recognized command ID
// };

// struct TimeoutEvent {};

// /// Strongly typed variant representing all possible speech events
// using SpeechEvent = std::variant<WakeWordEvent, CommandEvent, TimeoutEvent>;

// /**
//  * @brief Level 3 Engine: Pull-based Speech Recognition Engine.
//  * 
//  * Strictly owns Level 2 DSP Engine internally.
//  * Callers pull events directly via pollEvent().
//  */
// class SpeechEngine {
// public:
//     SpeechEngine();
//     ~SpeechEngine();

//     // Disable Copying and Moving explicitly
//     SpeechEngine(const SpeechEngine&) = delete;
//     SpeechEngine& operator=(const SpeechEngine&) = delete;
//     SpeechEngine(SpeechEngine&&) = delete;
//     SpeechEngine& operator=(SpeechEngine&&) = delete;

//     /**
//      * @brief Pulls next pending event from the voice state machine.
//      * 
//      * @return std::optional<SpeechEvent> Event if recognized/state changed, otherwise std::nullopt.
//      */
//     [[nodiscard]] std::optional<SpeechEvent> pollEvent();

// private:
//     enum class State {
//         ListeningForWakeWord,
//         ListeningForCommand
//     };

//     State current_state_{State::ListeningForWakeWord};

//     // Owned Level 2 DSP Processor
//     std::unique_ptr<AudioProcessing::AudioProcessor> audio_processor_;

//     // Vendor ML pointer handles directly in the class
//     srmodel_list_t* models_{nullptr};
//     esp_mn_iface_s* multinet_{nullptr};
//     model_iface_data_s* model_data_{nullptr};
// };

// } // namespace Speech