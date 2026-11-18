#pragma once

// Adapter: core::IButtonInput over the board key driver.
// The debounce/long-press decision is made by general/input/ButtonStateMachine;
// this adapter only maps physical keys/events to the core enums.

#include "interface.hpp"

#include "button_driver.h"

namespace adapters {

class EspButtonInput;

inline void espKeyHandler(key_id_t id, key_event_t event, void* user_data);

class EspButtonInput : public core::IButtonInput {
public:
    bool start(Handler handler, void* ctx) override {
        handler_ = handler;
        ctx_ = ctx;

        // Register the callback first, then start the scan task (the task can
        // deliver events from the very first scan).
        if (key_register_callback(&espKeyHandler) != ESP_OK) {
            return false;
        }
        // The driver forwards this pointer back as the callback's user_data.
        return key_module_init(this) == ESP_OK;
    }

    void handle(key_id_t id, key_event_t event) {
        if (handler_ == nullptr) {
            return;
        }

        core::ButtonId button;
        switch (id) {
            case KEY_ID_9:  button = core::ButtonId::Previous; break;
            case KEY_ID_10: button = core::ButtonId::PlayPause; break;
            case KEY_ID_11: button = core::ButtonId::Next; break;
            default: return;
        }

        const core::ButtonEvent ev = (event == KEY_EVENT_LONG_PRESS)
            ? core::ButtonEvent::LongPress
            : core::ButtonEvent::ShortPress;
        handler_(button, ev, ctx_);
    }

private:
    Handler handler_ = nullptr;
    void* ctx_ = nullptr;
};

inline void espKeyHandler(key_id_t id, key_event_t event, void* user_data) {
    auto* self = static_cast<EspButtonInput*>(user_data);
    if (self != nullptr) {
        self->handle(id, event);
    }
}

} // namespace adapters
