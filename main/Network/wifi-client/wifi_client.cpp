#include "wifi_client.hpp"    // Include corresponding header file definitions
#include "esp_log.h"          // Include ESP-IDF logging library header
#include <cstring>            // Include standard C string manipulation library header
#include "freertos/FreeRTOS.h" // Include FreeRTOS base header for task delays
#include "freertos/task.h"       // Include FreeRTOS task management header

// [NEW] Static C-style event bridge function implementation
void WifiClient::wifi_event_bridge(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto* client = static_cast<WifiClient*>(arg);
    if (client) {
        client->handleSystemEvent(event_base, event_id, event_data);
    }
}

WifiClient::WifiClient() {    // Constructor definition (no complex state tracking needed here)
    // [NEW] Register system event listeners, capturing instance handles for safe unregistration
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiClient::wifi_event_bridge, this, &wifi_event_instance);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiClient::wifi_event_bridge, this, &ip_event_instance);
}

WifiClient::~WifiClient() {   // Destructor definition for clean lifecycle exit
    // [NEW] Unregister using exact instance handles to prevent memory leaks and API mismatch errors
    if (wifi_event_instance) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_instance);
    }
    if (ip_event_instance) {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_instance);
    }
    disconnect();             // Ensure disconnection occurs on destruction
}                             // End of destructor definition

// [NEW] Passive event handler method storing system events into internal state flags
void WifiClient::handleSystemEvent(esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        connected_flag.store(true);
        disconnect_reason.store(0); // Clear error on successful connection
        ESP_LOGI(TAG, "Wi-Fi client successfully obtained IP address.");

 // Check if we are currently in APSTA mode before switching back
        wifi_mode_t current_mode;
        if (esp_wifi_get_mode(&current_mode) == ESP_OK && current_mode == WIFI_MODE_APSTA) {
            esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Switched back to pure STA mode. Provisioning AP stopped.");
            } else {
                ESP_LOGE(TAG, "Failed to switch back to STA mode: %s", esp_err_to_name(ret));
            }
        }

    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        connected_flag.store(false);
        auto* event = static_cast<wifi_event_sta_disconnected_t*>(event_data);
        if (event) {
            disconnect_reason.store(event->reason); // Store the exact raw reason code provided by the driver
            ESP_LOGW(TAG, "Wi-Fi disconnected. Raw reason code: %d", event->reason);
        }
    }
}

bool WifiClient::connect(const std::string& ssid, const std::string& password) {
    if (ssid.empty()) {       // Check if provided SSID string is empty
        ESP_LOGE(TAG, "Cannot connect: SSID is empty."); // Log error message
        return false;         // Return failure status boolean
    }                         // End of SSID check block

    // [NEW] Clear out stale history and reset state from previous attempts before starting fresh
    disconnect_reason.store(0);
    connected_flag.store(false);
    
    // [NEW] Ensure any lingering connection attempt is forcefully cleared out first!
    esp_wifi_disconnect();


    wifi_config_t wifi_config = {}; // Initialize Wi-Fi configuration structure with zeroes
    std::strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid)); // Copy SSID safely
    std::strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password)); // Copy Password safely


esp_err_t ret;
// Check current mode first to avoid redundant driver re-inits
    wifi_mode_t current_mode;
    if (esp_wifi_get_mode(&current_mode) == ESP_OK) {
        if (current_mode != WIFI_MODE_APSTA) {
              ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set Wi-Fi APSTA mode: %s", esp_err_to_name(ret));
                return false;
            }
            ESP_LOGI(TAG, "Switched to APSTA mode for credential testing.");
        }
    }





    // esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA); // Set Wi-Fi operating mode to Station (STA)
    // if (ret != ESP_OK) {      // Check if setting mode failed
    //     ESP_LOGE(TAG, "Failed to set Wi-Fi STA mode: %s", esp_err_to_name(ret)); // Log error details
    //     return false;         // Return failure status boolean
    // }                         // End of mode error check block

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config); // Apply SSID and password configuration to interface
    if (ret != ESP_OK) {      // Check if configuration assignment failed
        ESP_LOGE(TAG, "Failed to set Wi-Fi configuration: %s", esp_err_to_name(ret)); // Log error details
        return false;         // Return failure status boolean
    }                         // End of config error check block

    ESP_LOGI(TAG, "Triggering connection to SSID: [%s]...", ssid.c_str()); // Log connection progress
    ret = esp_wifi_connect();   // Call ESP-IDF driver API to initiate connection sequence
    if (ret != ESP_OK) {        // Check if connection trigger failed
        ESP_LOGE(TAG, "Failed to trigger esp_wifi_connect: %s", esp_err_to_name(ret)); // Log error details
        return false;           // Return failure status boolean
    }                           // End of connect trigger error check block

    return true;                // Return success boolean indicating connection command was accepted
}                             // End of connect method

void WifiClient::disconnect() { // Method to terminate current Wi-Fi connection
    esp_wifi_disconnect();    // Call ESP-IDF driver disconnection API
    connected_flag.store(false); // [NEW] Clear connected status flag on explicit disconnect
    ESP_LOGI(TAG, "WifiClient disconnected."); // Log disconnection notice message
}                             // End of disconnect method

// bool WifiClient::isConnected() const { // Method to query active connection status
//     wifi_ap_record_t ap_info;   // Initialize access point information record structure
//     // Check if the station successfully fetched current connected AP info (indicates active association)
//     if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
//         return true;            // Return true if connected to an AP
//     }                           // End of AP info check block
//     return false;               // Return false if not connected
// }                             // End of isConnected method

bool WifiClient::isConnected() const {
    // [NEW] Check atomic cached event flag first for thread-safe speed
    if (connected_flag.load()) {
        return true;
    }

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