 

 

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
#include "protocol/client/wifi_got_ip_protocol.hpp"
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
        // 1. Populate the cross-platform struct
        Protocol::Wifi::GotIpInfo info;
 info.ip      = status.ip;
info.gateway = status.gateway;
info.netmask = status.netmask;
info.dns     = status.dns;
info.mac     = status.mac;
        info.quality = status.quality;

        // 2. Encode to raw bytes
        std::vector<uint8_t> payload = Protocol::Wifi::encode_got_ip(info);

        // 3. Dispatch via your thread-safe Network model
        Network::send(payload);

    } else {
        ESP_LOGW(TAG, "Failed to retrieve full WiFi status during got_ip event");
    }
}


} // namespace wifi