 

 

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





// /**
//  * @brief Native ESP-IDF Callback for got IP event (Thread-Safe / Race-Free)
//  * @param arg User-defined argument passed during registration
//  * @param event_base Event base category (IP_EVENT)
//  * @param event_id Specific event identifier (IP_EVENT_STA_GOT_IP)
//  * @param event_data Pointer to native event data structure (ip_event_got_ip_t)
//  */
// void on_wifi_got_ip(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
// {
//     ESP_LOGI(TAG, "+++++++++++++++++++++++++++++++WiFi got IP event received natively");

//     // 1. Cast the native event data provided directly in the callback argument
//     ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

//     // 2. Initialize your status structure safely
//     wifi_status_t status = {};

//     // Populate IP info directly from the event payload
//     esp_ip4_addr_t ip = event->ip_info.ip;
//     esp_ip4_addr_t netmask = event->ip_info.netmask;
//     esp_ip4_addr_t gateway = event->ip_info.gw;

//     snprintf(status.ip, sizeof(status.ip), IPSTR, IP2STR(&ip));
//     snprintf(status.netmask, sizeof(status.netmask), IPSTR, IP2STR(&netmask));
//     snprintf(status.gateway, sizeof(status.gateway), IPSTR, IP2STR(&gateway));

//     // Fetch MAC address safely
//     uint8_t mac[6];
//     if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
//         snprintf(status.mac, sizeof(status.mac), "%02X:%02X:%02X:%02X:%02X:%02X",
//                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//     }

//     // Fetch current AP info (RSSI, SSID, channel) thread-safely via esp_wifi API
//     wifi_ap_record_t ap_info;
//     if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
//         memcpy(status.ssid, ap_info.ssid, sizeof(status.ssid));
//         status.ssid[sizeof(status.ssid) - 1] = '\0';
        
//         memcpy(status.bssid, ap_info.bssid, sizeof(status.bssid));
//         status.rssi = ap_info.rssi;
//         status.channel = ap_info.primary;
        
//         // Calculate a basic signal quality percentage from RSSI (-100 to -50 dBm scale)
//         if (ap_info.rssi <= -100) status.quality = 0;
//         else if (ap_info.rssi >= -50) status.quality = 100;
//         else status.quality = (uint8_t)(2 * (ap_info.rssi + 100));
//     }

//     // Populate uptime
//     status.uptime_ms = (uint32_t)(esp_timer_get_time() / 1000);

//     // 3. Log the safe details
//     ESP_LOGI(TAG, "IP: %s", status.ip);
//     ESP_LOGI(TAG, "Gateway: %s", status.gateway);
//     ESP_LOGI(TAG, "Netmask: %s", status.netmask);
//     ESP_LOGI(TAG, "MAC: %s", status.mac);
//     ESP_LOGI(TAG, "Signal quality: %d%%", status.quality);

//     // 4. Save to persistent file storage safely
//     if (wifiConditionRecorderFile) {
//         wifiConditionRecorderFile->save(status);
//     }
// }

} // namespace wifi