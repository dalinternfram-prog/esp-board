#pragma once

// WiFi connected event: log only. Protocol notifications belong to the core.

#include <cstddef>

#include "esp_log.h"
#include "esp_wifi_manager.h"

#include "wifi/shared.hpp"

namespace wifi {

inline void on_wifi_connected(const char *event, const void *data, size_t /*len*/, void * /*ctx*/)
{
    const auto *info = static_cast<const wifi_connected_t *>(data);
    ESP_LOGI(TAG, "WiFi connected event [%s] to SSID: %s (RSSI: %d dBm, Channel: %d)",
             event, info->ssid, info->rssi, info->channel);
}

} // namespace wifi
