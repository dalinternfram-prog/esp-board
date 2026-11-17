#pragma once

// Adapter: hardware identity for the ESP-IDF platform (IDeviceInfo port).
// MAC/chip discovery stays here; string formatting lives in general/util/format.

#include "interface.hpp"

#include "esp_mac.h"

#include <string>

#include "general/util/format.hpp"

namespace adapters {

class EspDeviceInfo : public core::IDeviceInfo {
public:
    std::string mac() const override {
        uint8_t m[6] = {};
        if (esp_read_mac(m, ESP_MAC_WIFI_STA) != ESP_OK) {
            return {};
        }
        return general::util::formatMac(m);
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
        return general::util::formatDeviceId(chip(), m);
    }
};

} // namespace adapters

