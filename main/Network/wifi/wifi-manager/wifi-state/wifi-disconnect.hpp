  

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
#include "protocol/client/wifi/wifi_disconnected_protocol.hpp"
#include "wifi/shared.hpp"

#include "Net manager.hpp"

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
    

    // 1. Populate the protocol struct using the enum
    Protocol::Wifi::DisconnectedInfo dc_info;
    dc_info.ssid         =  reinterpret_cast<const char*>(info->ssid)  ;
    dc_info.reason       = static_cast<Protocol::Wifi::DisconnectReason>(info->reason);
    dc_info.timestamp_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

    // 2. Encode to protocol byte payload
    std::vector<uint8_t> payload = Protocol::Wifi::encode_disconnected(dc_info);

    // 3. Send via network model (safely buffers to Flash FIFO while offline!)
    Network::send(payload);

    ESP_LOGW(TAG, "WiFi disconnected from %s, reason: %s. Event queued to Flash FIFO.",
             dc_info.ssid.c_str(),
             Protocol::Wifi::to_string(dc_info.reason));
}

} // namespace wifi