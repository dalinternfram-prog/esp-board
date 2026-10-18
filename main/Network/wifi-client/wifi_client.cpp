#include "wifi_client.hpp"    // Include corresponding header file definitions
#include "esp_log.h"          // Include ESP-IDF logging library header
#include <cstring>            // Include standard C string manipulation library header
#include "freertos/FreeRTOS.h" // Include FreeRTOS base header for task delays
#include "freertos/task.h"       // Include FreeRTOS task management header

WifiClient::WifiClient() {}   // Constructor definition (no complex state tracking needed here)

WifiClient::~WifiClient() {   // Destructor definition for clean lifecycle exit
    disconnect();             // Ensure disconnection occurs on destruction
}                               // End of destructor definition

bool WifiClient::connect(const std::string& ssid, const std::string& password) {
    if (ssid.empty()) {       // Check if provided SSID string is empty
        ESP_LOGE(TAG, "Cannot connect: SSID is empty."); // Log error message
        return false;         // Return failure status boolean
    }                           // End of SSID check block

    wifi_config_t wifi_config = {}; // Initialize Wi-Fi configuration structure with zeroes
    std::strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid)); // Copy SSID safely
    std::strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password)); // Copy Password safely

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA); // Set Wi-Fi operating mode to Station (STA)
    if (ret != ESP_OK) {      // Check if setting mode failed
        ESP_LOGE(TAG, "Failed to set Wi-Fi STA mode: %s", esp_err_to_name(ret)); // Log error details
        return false;         // Return failure status boolean
    }                           // End of mode error check block

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config); // Apply SSID and password configuration to interface
    if (ret != ESP_OK) {      // Check if configuration assignment failed
        ESP_LOGE(TAG, "Failed to set Wi-Fi configuration: %s", esp_err_to_name(ret)); // Log error details
        return false;         // Return failure status boolean
    }                           // End of config error check block

    ESP_LOGI(TAG, "Triggering connection to SSID: [%s]...", ssid.c_str()); // Log connection progress
    ret = esp_wifi_connect();   // Call ESP-IDF driver API to initiate connection sequence
    if (ret != ESP_OK) {        // Check if connection trigger failed
        ESP_LOGE(TAG, "Failed to trigger esp_wifi_connect: %s", esp_err_to_name(ret)); // Log error details
        return false;           // Return failure status boolean
    }                           // End of connect trigger error check block

    return true;                // Return success boolean indicating connection command was accepted
}                               // End of connect method

void WifiClient::disconnect() { // Method to terminate current Wi-Fi connection
    esp_wifi_disconnect();    // Call ESP-IDF driver disconnection API
    ESP_LOGI(TAG, "WifiClient disconnected."); // Log disconnection notice message
}                               // End of disconnect method

// bool WifiClient::isConnected() const { // Method to query active connection status
//     wifi_ap_record_t ap_info;   // Initialize access point information record structure
//     // Check if the station successfully fetched current connected AP info (indicates active association)
//     if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
//         return true;            // Return true if connected to an AP
//     }                           // End of AP info check block
//     return false;               // Return false if not connected
// }                               // End of isConnected method



bool WifiClient::isConnected() const {
    // 1. Check if we have a default STA network interface
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) return false;

    // 2. Fetch network interface IP info
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        // If the IP address is not 0.0.0.0, we are fully connected and have an IP!
        if (ip_info.ip.addr != 0) {
            return true;
        }
    }
    
    return false;
}