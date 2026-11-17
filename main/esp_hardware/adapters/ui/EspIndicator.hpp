#pragma once

// Adapter: core::IIndicator over the RGB LED driver.

#include "interface.hpp"

#include "rgb_led_driver.h"

namespace adapters {

class EspIndicator : public core::IIndicator {
public:
    void setMode(core::IndicatorMode mode) override {
        switch (mode) {
            case core::IndicatorMode::Idle:         set_rgb_mode(RGB_MODE_IDLE); break;
            case core::IndicatorMode::Playing:      set_rgb_mode(RGB_MODE_PLAYING); break;
            case core::IndicatorMode::Recording:    set_rgb_mode(RGB_MODE_REC_COMMAND); break;
            case core::IndicatorMode::VoiceCommand: set_rgb_mode(RGB_MODE_REC_COMMAND); break;
        }
    }

    void setColor(core::IndicatorColor color) override {
        switch (color) {
            case core::IndicatorColor::Red:   set_rgb_color(RGB_COLOR_RED); break;
            case core::IndicatorColor::Green: set_rgb_color(RGB_COLOR_GREEN); break;
            case core::IndicatorColor::Blue:  set_rgb_color(RGB_COLOR_BLUE); break;
            case core::IndicatorColor::White: set_rgb_color(RGB_COLOR_WHITE); break;
        }
    }
};

} // namespace adapters
