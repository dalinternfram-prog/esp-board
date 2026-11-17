#pragma once

// ESP-IDF implementations of the platform ports (IClock / IRandom / ILogger).

#include "core/ports.hpp"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

namespace adapters {

class EspClock : public core::IClock {
public:
    uint32_t nowMs() const override {
        return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    }
};

class EspRandom : public core::IRandom {
public:
    uint32_t u32() override { return static_cast<uint32_t>(esp_random()); }
};

class EspLogger : public core::ILogger {
public:
    void log(core::LogLevel level, const char* msg) override {
        switch (level) {
            case core::LogLevel::Info:  ESP_LOGI("CORE", "%s", msg); break;
            case core::LogLevel::Warn:  ESP_LOGW("CORE", "%s", msg); break;
            case core::LogLevel::Error: ESP_LOGE("CORE", "%s", msg); break;
        }
    }
};

} // namespace adapters
