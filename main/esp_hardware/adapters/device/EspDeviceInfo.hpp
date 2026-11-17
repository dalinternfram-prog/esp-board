#pragma once

// Adapter: hardware identity for the ESP-IDF platform (IDeviceInfo port).

#include "interface.hpp"

#include "esp_mac.h"

#include <cstdio>
#include <string>

namespace adapters {

class EspDeviceInfo : public core::IDeviceInfo {
public:
    std::string mac() const override {
        uint8_t m[6] = {};
        if (esp_read_mac(m, ESP_MAC_WIFI_STA) != ESP_OK) {
            return {};
        }
        char buf[18];
        std::snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                      m[0], m[1], m[2], m[3], m[4], m[5]);
        return std::string(buf);
    }

    std::string chip() const override {
#ifdef CONFIG_IDF_TARGET
        return CONFIG_IDF_TARGET;   // e.g. "esp32s3"
#else
        return "unknown";
#endif
    }

    std::string deviceId() const override {
        uint8_t m[6] = {};
        if (esp_read_mac(m, ESP_MAC_WIFI_STA) != ESP_OK) {
            return "unknown";
        }
        char buf[40];
        std::snprintf(buf, sizeof(buf), "%s-%02X%02X%02X",
                      chip().c_str(), m[3], m[4], m[5]);
        return std::string(buf);
    }
};

} // namespace adapters
