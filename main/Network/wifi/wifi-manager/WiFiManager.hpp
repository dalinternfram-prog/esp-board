

#pragma once
#include "esp_bus.h" 
#include "esp_wifi_manager.h"
#include <string>
#include <memory>
#include "esp_log.h"
#include "nvs_flash.h"


namespace wifi {

// Initialize using the library's built-in multi-network, NVS, and HTTP features
void begin(const std::string& fallbackAPName = "ESP32_Config_AP");

// GET function to check whether Wi-Fi is successfully connected (using library API)
inline bool isConnected() {
    return wifi_manager_is_connected();
}

// Get detailed status (IP, RSSI, SSID) using the library's native status function
inline bool getStatus(wifi_status_t& statusOut) {
    return wifi_manager_get_status(&statusOut) == ESP_OK;
}

} // namespace wifi_mgr