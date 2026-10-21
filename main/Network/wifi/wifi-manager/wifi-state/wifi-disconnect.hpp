  

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
    
    // Log the disconnection event details, including SSID and the reason code
    ESP_LOGW(TAG, "+++++++++++++++++++++++++++++++WiFi disconnected event tag [%s] from %s (reason code: %d)", 
             event, info->ssid, info->reason);

    // Check if the file recorder instance is valid before attempting to save
    if (wifiConditionRecorderFile) {
        // Get current uptime in milliseconds using ESP-IDF timer
        uint32_t current_time_ms = (uint32_t)(esp_timer_get_time() / 1000);

        // Package the disconnection info and timestamp into our wrapper struct and save it
        wifiConditionRecorderFile->save(disconnected_wifi{*info, current_time_ms});
    }
}

} // namespace wifi