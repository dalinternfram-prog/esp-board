#pragma once

// Shared WiFi helpers: logging tag + the single entry point used by the link
// adapter. Historical priority/cleanup policy was removed (unused).

#include "esp_wifi_manager.h"
#include "WiFiManager.hpp"

namespace wifi {

static const char* TAG = "WIFI__";

inline void init() {
    wifi::begin();
}

} // namespace wifi
