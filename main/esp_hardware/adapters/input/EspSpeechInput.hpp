#pragma once

// Adapter: core::ISpeechInput over the ESP-SR callback.
// The speech driver invokes its callback with user_data == NULL, so the adapter
// keeps a singleton pointer to the active instance.

#include "interface.hpp"

#include "esp_log.h"
#include "mic_speech.h"

namespace adapters {

class EspSpeechInput;

inline void espSpeechHandler(esp_sr_rec_event_t event, esp_sr_evt_data_t data,
                             void* user_data);

class EspSpeechInput : public core::ISpeechInput {
public:
    bool start(Handler handler, void* ctx) override {
        handler_ = handler;
        ctx_ = ctx;
        instance() = this;

        // Speech is deliberately disabled for now:
        //  * Speech_Init() requires a "model" partition (not in partitions.csv)
        //  * its feed task would fight AudioCaptureDriver for the same I2S RX
        // Re-enable by provisioning the model partition and routing both
        // consumers through one capture stream.
        ESP_LOGW("speech", "Speech input disabled (no model partition / shared I2S)");
        return false;
    }

    void handle(esp_sr_rec_event_t event, esp_sr_evt_data_t data) {
        if (handler_ == nullptr) {
            return;
        }

        switch (event) {
            case ESP_SR_EVT_AWAKEN:
                handler_(core::SpeechEvent::Awake, 0, ctx_);
                break;
            case ESP_SR_EVT_CMD:
                handler_(core::SpeechEvent::Command, static_cast<int>(data.sr_cmd), ctx_);
                break;
            case ESP_SR_EVT_CMD_TIMEOUT:
                handler_(core::SpeechEvent::CommandTimeout, 0, ctx_);
                break;
            default:
                break;
        }
    }

    static EspSpeechInput*& instance() {
        static EspSpeechInput* s_instance = nullptr;
        return s_instance;
    }

private:
    Handler handler_ = nullptr;
    void* ctx_ = nullptr;
};

inline void espSpeechHandler(esp_sr_rec_event_t event, esp_sr_evt_data_t data,
                             void* /*user_data*/) {
    auto* self = EspSpeechInput::instance();
    if (self != nullptr) {
        self->handle(event, data);
    }
}

} // namespace adapters
