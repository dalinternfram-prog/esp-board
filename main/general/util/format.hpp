#pragma once

// Portable identity/text formatting. No ESP-IDF / FreeRTOS dependencies.

#include <cstdint>
#include <cstdio>
#include <string>

namespace general::util {

inline std::string formatMac(const uint8_t mac[6]) {
    char buf[18];
    std::snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

inline std::string formatDeviceId(const std::string& chip, const uint8_t mac[6]) {
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%s-%02X%02X%02X",
                  chip.c_str(), mac[3], mac[4], mac[5]);
    return std::string(buf);
}

} // namespace general::util
