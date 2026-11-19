#pragma once

// Portable short/long-press state machine.
// Debounce is the caller's sampling interval (the ESP driver polls every
// 100 ms, which is well above the mechanical bounce time); this class only
// derives events from the sampled level and timestamp.

#include <cstdint>

namespace general::input {

enum class ButtonEvent {
    None,
    ShortPress,
    LongPress,
};

class ButtonStateMachine {
public:
    explicit ButtonStateMachine(uint32_t long_press_ms = 1000);

    // Call on every sample. Returns the event produced by this transition.
    ButtonEvent update(bool pressed, uint32_t now_ms);

private:
    uint32_t long_press_ms_;
    bool     pressed_ = false;
    bool     last_pressed_ = false;
    bool     long_triggered_ = false;
    uint32_t press_start_ms_ = 0;
};

} // namespace general::input
