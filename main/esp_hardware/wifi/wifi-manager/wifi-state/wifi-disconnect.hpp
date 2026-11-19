  

#pragma once

#include "esp_bus.h" 
#include "esp_timer.h"
#include "esp_wifi_manager.h"
#include <cstdint>
#include <string>
#include <memory>
#include <cstring>
#include <chrono>
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi/shared.hpp"

namespace wifi {
 



 /**
 * @brief Callback for WiFi disconnected event
 * @param event Event identifier string (e.g., "disconnected")
 * @param data Pointer to event-specific data structure (wifi_disconnected_t)
 * @param len Length of data payload
 * @param ctx User-defined context pointer
 */
inline void on_wifi_disconnected(const char *event, const void *data, size_t len, void *ctx)
{
    // Cast the incoming raw data pointer to the library's wifi_disconnected_t structure
    const wifi_disconnected_t *info = (const wifi_disconnected_t *)data;
    

    // Log the disconnect event. No protocol::IoT "disconnected" message exists
    // yet, so we intentionally do NOT emit a non-conforming packet here.
    ESP_LOGW(TAG, "WiFi disconnected from %s, reason: %u",
             info->ssid, static_cast<unsigned>(info->reason));

    // TODO(proto): once components/proto defines the "disconnected" message,
    // encode via protocol::IoT::client::encode...(...) and INetwork::send(...).
}

} // namespace wifi