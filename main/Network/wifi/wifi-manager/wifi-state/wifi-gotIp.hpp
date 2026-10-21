 

 

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
#include "esp_mac.h"
#include "esp_wifi.h"

namespace wifi {
 

 /**
 * @brief Callback for got IP event
 * @param event Event identifier string (e.g., "got_ip")
 * @param data Pointer to event-specific data structure (esp_netif_ip_info_t)
 * @param len Length of data payload
 * @param ctx User-defined context pointer
 */
inline void on_wifi_got_ip(const char *event, const void *data, size_t len, void *ctx)
{
    // Log that the IP acquisition event has fired
    ESP_LOGI(TAG, "+++++++++++++++++++++++++++++++WiFi got IP event tag [%s] received", event);

    // Retrieve the full current status object from the wifi manager library
    wifi_status_t status;
    if (wifi_manager_get_status(&status) == ESP_OK) {
        // Log details of the assigned IP parameters and link stats
        ESP_LOGI(TAG, "IP: %s", status.ip);
        ESP_LOGI(TAG, "Gateway: %s", status.gateway);
        ESP_LOGI(TAG, "Netmask: %s", status.netmask);
        ESP_LOGI(TAG, "DNS: %s", status.dns);
        ESP_LOGI(TAG, "MAC: %s", status.mac);
        ESP_LOGI(TAG, "Signal quality: %d%%", status.quality);

        // Check if the file recorder instance is valid before attempting to save
        if (wifiConditionRecorderFile) {
            // Package the full status into our struct wrapper and save it persistently
            wifiConditionRecorderFile->save(status);
        }
    } else {
        ESP_LOGW(TAG, "Failed to retrieve full WiFi status during got_ip event");
    }
}


} // namespace wifi