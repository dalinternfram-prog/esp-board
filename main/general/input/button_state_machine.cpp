#include "general/input/button_state_machine.hpp"

namespace general::input {

ButtonStateMachine::ButtonStateMachine(uint32_t long_press_ms)
    : long_press_ms_(long_press_ms) {}

ButtonEvent ButtonStateMachine::update(bool pressed, uint32_t now_ms) {
    ButtonEvent event = ButtonEvent::None;
    pressed_ = pressed;

    if (pressed_ && !last_pressed_) {
        // Just pressed: remember when.
        press_start_ms_ = now_ms;
        long_triggered_ = false;
    } else if (!pressed_ && last_pressed_) {
        // Released: a short press only if the long press did not fire.
        const uint32_t press_time = now_ms - press_start_ms_;
        if (press_time < long_press_ms_ && !long_triggered_) {
            event = ButtonEvent::ShortPress;
        }
    } else if (pressed_ && !long_triggered_) {
        // Held down: fire the long press exactly once.
        if (now_ms - press_start_ms_ >= long_press_ms_) {
            event = ButtonEvent::LongPress;
            long_triggered_ = true;
        }
    }

    last_pressed_ = pressed_;
    return event;
}

} // namespace general::input
