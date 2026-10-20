#pragma once
#include "Database/class-json/json-file.hpp"
#include "wifi/shared.hpp"

#include "esp_wifi_manager.h"
#include "WiFiManager.hpp"


// Define the maximum threshold of saved Wi-Fi networks allowed in NVS
#define MAX_SAVED_WIFI_NETWORKS 10




namespace wifi {



struct connected_wifi{
wifi_connected_t source;
uint32_t time;
};
struct disconnected_wifi{
wifi_disconnected_t source;
uint32_t time;
};

static const char* TAG = "WIFI__";





inline std::shared_ptr<file::JsonFileStore> wifiConditionRecorderFile;
inline std::string fileName = "wifiConditionRecorder";






inline void init(){

        wifiConditionRecorderFile = std::make_unique<file::JsonFileStore>(fileName );
    wifi::begin();
}




/**
 * @brief Helper function to upgrade/increase the priority of a connected network
 * @param ssid Target Wi-Fi network SSID string
 */
inline void priorityUpgrade(const char *ssid)
{
    wifi_network_t net;
    
    // Retrieve the existing network configuration object from NVS storage
    if (wifi_manager_get_network(ssid, &net) == ESP_OK) {
        // Check if the current network's priority has maxed out at 255
        if (net.priority >= 255) {
            net.priority = 255; 
            ESP_LOGI(TAG, "Network '%s' reached max priority (255). Lowering other networks' priorities...", ssid);
            
            // Dynamically fetch count to allocate exact size without hardcoding
            size_t count = 0;
            if (wifi_manager_list_networks(nullptr, 0, &count) == ESP_OK && count > 0) {
                wifi_network_t* networks = new wifi_network_t[count];
                
                if (wifi_manager_list_networks(networks, count, &count) == ESP_OK) {
                    for (size_t i = 0; i < count; ++i) {
                        // Skip the active connected network
                        if (std::string(networks[i].ssid) == std::string(ssid)) {
                            continue;
                        }
                        
                        // Decrease priority, ensuring it doesn't drop below 0
                        if (networks[i].priority > 0) {
                            networks[i].priority -= 1;
                            wifi_manager_update_network(&networks[i]);
                        }
                    }
                }
                delete[] networks;
            }
            
        } else {
            // Increment priority normally if below 255
            net.priority += 1;
        }
        
        // Save and update the current network back to NVS storage
        if (wifi_manager_update_network(&net) == ESP_OK) {
            ESP_LOGI(TAG, "Upgraded priority for network '%s' to %u", net.ssid, net.priority);
        } else {
            ESP_LOGW(TAG, "Failed to update priority for network '%s'", net.ssid);
        }
        
    } else {
        ESP_LOGD(TAG, "Network '%s' not found in saved list (connected via fallback)", ssid);
    }
}

/**
 * @brief Cleans up saved networks from NVS if capacity exceeds threshold and they have never connected.
 * @param current_ssid The currently active connected SSID (protected from deletion).
 */
inline void cleanup_unconnected_networks(const char *current_ssid)
{
    if (!wifiConditionRecorderFile) return;

    size_t count = 0;
    // Query exact number of saved networks dynamically
    if (wifi_manager_list_networks(nullptr, 0, &count) != ESP_OK || count == 0) {
        return;
    }

    // Check if saved network count exceeds our defined threshold limit
    if (count > MAX_SAVED_WIFI_NETWORKS) {
        wifi_network_t* all_networks = new wifi_network_t[count];
        
        if (wifi_manager_list_networks(all_networks, count, &count) == ESP_OK) {
            auto savedSize = wifiConditionRecorderFile->GetSize<wifi_status_t>();

            for (size_t j = 0; j < count; ++j) {
                bool connected_before = false;

                // Cross-reference against historical connection logs stored as wifi_status_t entries
                for (int k = 0; k < savedSize; ++k) {
                    auto past_status = wifiConditionRecorderFile->fetch<wifi_status_t>(k);
                    if (std::strcmp(all_networks[j].ssid, past_status.ssid) == 0) {
                        connected_before = true;
                        break;
                    }
                }

                // If saved in NVS, never connected before, and is not the active connection, remove it
                if (!connected_before && std::strcmp(all_networks[j].ssid, current_ssid) != 0) {
                    ESP_LOGW(TAG, "Removing unused saved network '%s' due to NVS capacity limit", all_networks[j].ssid);
                    wifi_manager_remove_network(all_networks[j].ssid);
                    break; // Exit loop after one removal cycle for safety
                }
            }
        }
        delete[] all_networks;
    }
}

/**
 * @brief Placeholder function to inspect waste or low-priority slots/data items.
 */
inline void inspect_waste_slot()
{
    // Implementation placeholder for future audits
}



}

 