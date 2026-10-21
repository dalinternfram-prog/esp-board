// // // #pragma once



// // // #include "esp_bus.h" 
// // // #include "esp_wifi_manager.h"
// // // #include <string>
// // // #include <memory>
// // // #include "esp_log.h"
// // // #include "nvs_flash.h"

// // // #include "shared.hpp"




// // // namespace wifi {



// // //  struct WifiConnectedEvent {
// // //     char ssid[32];
// // //     char bssid[18];       // MAC address string
// // //     int rssi;
// // //     int channel;
// // //     int quality;
// // //     int64_t timestamp_ms; // Epoch time in milliseconds

// // //     // Constructor to populate detailed fields including password
// // //     WifiConnectedEvent(const char* ssid_str, const char* pass_str, const uint8_t* bssid_raw, int rssi_val, int channel_val, int quality_val = 0)
// // //         : rssi(rssi_val), 
// // //           channel(channel_val), 
// // //           quality(quality_val),
// // //           timestamp_ms(std::chrono::duration_cast<std::chrono::milliseconds>(
// // //               std::chrono::system_clock::now().time_since_epoch()).count()) {
        
// // //         // Safely copy SSID
// // //         if (ssid_str) {
// // //             std::strncpy(ssid, ssid_str, sizeof(ssid) - 1);
// // //             ssid[sizeof(ssid) - 1] = '\0';
// // //         } else {
// // //             ssid[0] = '\0';
// // //         }

// // //         // Safely copy Password
// // //         if (pass_str) {
// // //             std::strncpy(password, pass_str, sizeof(password) - 1);
// // //             password[sizeof(password) - 1] = '\0';
// // //         } else {
// // //             password[0] = '\0';
// // //         }

// // //         // Format BSSID MAC address
// // //         if (bssid_raw) {
// // //             std::snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
// // //                           bssid_raw[0], bssid_raw[1], bssid_raw[2],
// // //                           bssid_raw[3], bssid_raw[4], bssid_raw[5]);
// // //         } else {
// // //             bssid[0] = '\0';
// // //         }
// // //     }
// // // };



// // //  /**
// // //  * @brief Callback for WiFi connected event
// // //  */
// // // void on_wifi_connected(const char *event, const void *data, size_t len, void *ctx)
// // // {
// // //     const wifi_connected_t *info = (const wifi_connected_t *)data;
// // //     ESP_LOGI(TAG, "WiFi connected to %s (RSSI: %d dBm, Channel: %d)",
// // //              info->ssid, info->rssi, info->channel);

// // //     // Call priorityUpgrade to increase the priority of the successfully connected network
// // //     priorityUpgrade(info->ssid);

// // // wifiConditionRecorderFile->save(WifiConnectedEvent);

// // // auto savedSize = wifiConditionRecorderFile->GetSize<WifiConnectedEvent>();
// // // for(int i =0;i<savedSize;++i){
// // //     auto temp_ = wifiConditionRecorderFile->fetch<WifiConnectedEvent>(i);
// // //     // get the ssid,which means connected one,it's a history;
// // //     // find all saved nvs,if wifi saved but not connected before,we delete it if saved nvs exceed a number;;
// // // }
// // // }

// // // /**
// // //  * @brief Helper function to upgrade/increase the priority of a connected network
// // //  */
// // // void priorityUpgrade(const char *ssid)
// // // {
// // //     wifi_network_t net;
    
// // //     // Retrieve the existing network configuration from storage
// // //     if (wifi_manager_get_network(ssid, &net) == ESP_OK) {
// // //         // Increase priority (ensure it doesn't overflow 255)
// // //         if (net.priority < 255) {
// // //             net.priority += 1; // Or add your desired boost amount
// // //         }
        
// // //         // Update the network in storage (NVS)
// // //         if (wifi_manager_update_network(&net) == ESP_OK) {
// // //             ESP_LOGI(TAG, "Upgraded priority for network '%s' to %u", net.ssid, net.priority);
// // //         } else {
// // //             ESP_LOGW(TAG, "Failed to update priority for network '%s'", net.ssid);
// // //         }
// // //     } else {
// // //         ESP_LOGD(TAG, "Network '%s' not found in saved list (might be connected via default fallback)", ssid);
// // //     }
// // // }




// // // void inspect_waste_slot(){

// // // }














// // // }



















// // #pragma once

// // #include "esp_bus.h" 
// // #include "esp_wifi_manager.h"
// // #include <string>
// // #include <memory>
// // #include <cstring>
// // #include <chrono>
// // #include "esp_log.h"
// // #include "nvs_flash.h"
// // #include "shared.hpp"

// // // Define a maximum limit for saved Wi-Fi networks in NVS to prevent clutter
// // #define MAX_SAVED_WIFI_NETWORKS 10

// // namespace wifi {

// // // Forward declaration of the file recorder variable (assumed to be defined globally or as extern in your shared context)
// // extern std::shared_ptr<FileManager> wifiConditionRecorderFile;

// // // Forward declaration of TAG for logging inside namespace
// // extern const char* TAG;

// // /**
// //  * @struct WifiConnectedEvent
// //  * @brief Structure representing a Wi-Fi connection event payload for storage and history tracking.
// //  */
// // struct WifiConnectedEvent {
// //     char ssid[32];            // SSID of the connected Wi-Fi network
// //     char password[64];        // Password of the connected Wi-Fi network
// //     char bssid[18];           // MAC address string format (XX:XX:XX:XX:XX:XX)
// //     int rssi;                 // Signal strength in dBm
// //     int channel;              // Wi-Fi radio channel
// //     int quality;              // Signal quality percentage (0-100)
// //     int64_t timestamp_ms;     // Epoch time in milliseconds when connection occurred

// //     /**
// //      * @brief Constructor to populate detailed fields including password and timestamp.
// //      */
// //     WifiConnectedEvent(const char* ssid_str, const char* pass_str, const uint8_t* bssid_raw, int rssi_val, int channel_val, int quality_val = 0)
// //         : rssi(rssi_val), 
// //           channel(channel_val), 
// //           quality(quality_val),
// //           timestamp_ms(std::chrono::duration_cast<std::chrono::milliseconds>(
// //               std::chrono::system_clock::now().time_since_epoch()).count()) {
        
// //         // Safely copy and null-terminate SSID string
// //         if (ssid_str) {
// //             std::strncpy(ssid, ssid_str, sizeof(ssid) - 1);
// //             ssid[sizeof(ssid) - 1] = '\0';
// //         } else {
// //             ssid[0] = '\0';
// //         }

// //         // Safely copy and null-terminate Password string
// //         if (pass_str) {
// //             std::strncpy(password, pass_str, sizeof(password) - 1);
// //             password[sizeof(password) - 1] = '\0';
// //         } else {
// //             password[0] = '\0';
// //         }

// //         // Format raw BSSID bytes into a readable MAC address string
// //         if (bssid_raw) {
// //             std::snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
// //                           bssid_raw[0], bssid_raw[1], bssid_raw[2],
// //                           bssid_raw[3], bssid_raw[4], bssid_raw[5]);
// //         } else {
// //             bssid[0] = '\0';
// //         }
// //     }
// // };

// // // Forward declaration of priorityUpgrade helper function
// // void priorityUpgrade(const char *ssid);

// // /**
// //  * @brief Callback for WiFi connected event
// //  * @param event Event identifier string
// //  * @param data Pointer to event-specific data structure (wifi_connected_t)
// //  * @param len Length of data payload
// //  * @param ctx User-defined context pointer
// //  */
// // void on_wifi_connected(const char *event, const void *data, size_t len, void *ctx)
// // {
// //     // Cast the incoming raw data pointer to the library's wifi_connected_t structure
// //     const wifi_connected_t *info = (const wifi_connected_t *)data;
    
// //     // Log successful connection details including SSID, RSSI, and channel
// //     ESP_LOGI(TAG, "WiFi connected to %s (RSSI: %d dBm, Channel: %d)",
// //              info->ssid, info->rssi, info->channel);

// //     // Call priorityUpgrade to increase or scale priorities of stored networks
// //     priorityUpgrade(info->ssid);

// //     // Ensure the file recorder instance exists before attempting file operations
// //     if (wifiConditionRecorderFile) {
// //         // Instantiate a connection event object using current connection metadata
// //         // Note: Password can be fetched or left blank depending on what's available in 'info'
// //         WifiConnectedEvent connectionEvent(info->ssid, "", nullptr, info->rssi, info->channel);

// //         // Save the current connection event record to persistent file storage
// //         wifiConditionRecorderFile->save(connectionEvent);

// //         // Retrieve the total number of historical connection records stored
// //         auto savedSize = wifiConditionRecorderFile->GetSize<WifiConnectedEvent>();
        
// //         // Loop through all history records to cross-reference with saved NVS networks
// //         for (int i = 0; i < savedSize; ++i) {
// //             // Fetch the historical connection event entry at index i
// //             auto historyEntry = wifiConditionRecorderFile->fetch<WifiConnectedEvent>(i);
            
// //             // Fetch all currently saved networks from NVS storage to check status
// //             size_t max_networks = 20;
// //             wifi_network_t* all_networks = new wifi_network_t[max_networks];
// //             size_t count = 0;

// //             if (wifi_manager_list_networks(all_networks, max_networks, &count) == ESP_OK) {
// //                 // If total saved NVS networks exceed our defined threshold limit
// //                 if (count > MAX_SAVED_WIFI_NETWORKS) {
// //                     // Iterate through the saved NVS networks list
// //                     for (size_t j = 0; j < count; ++j) {
// //                         bool connected_before = false;

// //                         // Check if this particular saved network exists in our connection history logs
// //                         for (int k = 0; k < savedSize; ++k) {
// //                             auto past_event = wifiConditionRecorderFile->fetch<WifiConnectedEvent>(k);
// //                             if (std::strcmp(all_networks[j].ssid, past_event.ssid) == 0) {
// //                                 connected_before = true; // Mark as verified connection history
// //                                 break;
// //                             }
// //                         }

// //                         // If the network is saved in NVS but has NEVER been successfully connected to before,
// //                         // and we have exceeded our storage capacity, remove it to free up space.
// //                         if (!connected_before) {
// //                             // Make sure we do not delete the network we just successfully connected to
// //                             if (std::strcmp(all_networks[j].ssid, info->ssid) != 0) {
// //                                 ESP_LOGW(TAG, "Removing unused/unconnected saved network '%s' due to NVS capacity limit", all_networks[j].ssid);
// //                                 wifi_manager_remove_network(all_networks[j].ssid);
// //                                 break; // Break loop after removing one to re-evaluate state safely
// //                             }
// //                         }
// //                     }
// //                 }
// //             }
// //             // Free dynamically allocated temporary network array
// //             delete[] all_networks;
// //         }
// //     }
// // }

// // /**
// //  * @brief Helper function to upgrade/increase the priority of a connected network
// //  * @param ssid Target Wi-Fi network SSID string
// //  */
// // void priorityUpgrade(const char *ssid)
// // {
// //     wifi_network_t net;
    
// //     // Retrieve the existing network configuration object from NVS storage
// //     if (wifi_manager_get_network(ssid, &net) == ESP_OK) {
// //         // Check if the current network's priority has already maxed out at 255
// //         if (net.priority >= 255) {
// //             net.priority = 255; // Cap priority at maximum allowable value
// //             ESP_LOGI(TAG, "Network '%s' reached max priority (255). Lowering other networks' priorities...", ssid);
            
// //             // Allocate a buffer to fetch all saved networks for scaling down
// //             size_t max_networks = 20;
// //             wifi_network_t* networks = new wifi_network_t[max_networks];
// //             size_t count = 0;
            
// //             // Retrieve list of all saved networks from NVS
// //             if (wifi_manager_list_networks(networks, max_networks, &count) == ESP_OK) {
// //                 for (size_t i = 0; i < count; ++i) {
// //                     // Skip the currently active connected network so its priority stays high
// //                     if (std::string(networks[i].ssid) == std::string(ssid)) {
// //                         continue;
// //                     }
                    
// //                     // Decrease other networks' priority by 1, making sure it doesn't drop below 0
// //                     if (networks[i].priority > 0) {
// //                         networks[i].priority -= 1;
// //                         wifi_manager_update_network(&networks[i]); // Save adjusted priority back to NVS
// //                     }
// //                 }
// //             }
// //             // Release memory allocated for the network array
// //             delete[] networks;
            
// //         } else {
// //             // Increment priority normally if it is below 255
// //             net.priority += 1;
// //         }
        
// //         // Save and update the current network back to NVS storage with its new priority value
// //         if (wifi_manager_update_network(&net) == ESP_OK) {
// //             ESP_LOGI(TAG, "Upgraded priority for network '%s' to %u", net.ssid, net.priority);
// //         } else {
// //             ESP_LOGW(TAG, "Failed to update priority for network '%s'", net.ssid);
// //         }
        
// //     } else {
// //         // Log if network wasn't found in NVS (e.g., connected via fallback configuration)
// //         ESP_LOGD(TAG, "Network '%s' not found in saved list (might be connected via default fallback)", ssid);
// //     }
// // }

// // /**
// //  * @brief Placeholder function to inspect waste or low-priority slots/data items.
// //  */
// // void inspect_waste_slot()
// // {
// //     // Implementation placeholder for future resource or slot auditing
// // }

// // } // namespace wifi



















// #pragma once

// #include "esp_bus.h" 
// #include "esp_wifi_manager.h"
// #include <string>
// #include <memory>
// #include <cstring>
// #include <chrono>
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "shared.hpp"

// // Define the maximum threshold of saved Wi-Fi networks allowed in NVS
// #define MAX_SAVED_WIFI_NETWORKS 10

// namespace wifi {

// // Forward declaration of the file recorder variable
// extern std::shared_ptr<FileManager> wifiConditionRecorderFile;

// // Forward declaration of TAG for logging inside namespace
// extern const char* TAG;

//  /**
//  * @struct WifiConnectedEvent
//  * @brief Structure representing a Wi-Fi connection event payload for storage and history tracking (without password storage).
//  */
// struct WifiConnectedEvent {
//     char ssid[32];            // SSID of the connected Wi-Fi network
//     char bssid[18];           // MAC address string format (XX:XX:XX:XX:XX:XX)
//     int rssi;                 // Signal strength in dBm
//     int channel;              // Wi-Fi radio channel
//     int quality;              // Signal quality percentage (0-100)
//     int64_t timestamp_ms;     // Epoch time in milliseconds when connection occurred

//     /**
//      * @brief Constructor to populate detailed fields (excluding password) and timestamp.
//      */
//     WifiConnectedEvent(const char* ssid_str, const uint8_t* bssid_raw, int rssi_val, int channel_val, int quality_val = 0)
//         : rssi(rssi_val), 
//           channel(channel_val), 
//           quality(quality_val),
//           timestamp_ms(std::chrono::duration_cast<std::chrono::milliseconds>(
//               std::chrono::system_clock::now().time_since_epoch()).count()) {
        
//         // Safely copy and null-terminate SSID string
//         if (ssid_str) {
//             std::strncpy(ssid, ssid_str, sizeof(ssid) - 1);
//             ssid[sizeof(ssid) - 1] = '\0';
//         } else {
//             ssid[0] = '\0';
//         }

//         // Format raw BSSID bytes into a readable MAC address string
//         if (bssid_raw) {
//             std::snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
//                           bssid_raw[0], bssid_raw[1], bssid_raw[2],
//                           bssid_raw[3], bssid_raw[4], bssid_raw[5]);
//         } else {
//             bssid[0] = '\0';
//         }
//     }
// };
// // Forward declarations of helper functions
// void priorityUpgrade(const char *ssid);
// void cleanup_unconnected_networks(const char *current_ssid);

// /**
//  * @brief Callback for WiFi connected event (concise, handles basic info routing)
//  * @param event Event identifier string
//  * @param data Pointer to event-specific data structure (wifi_connected_t)
//  * @param len Length of data payload
//  * @param ctx User-defined context pointer
//  */
// void on_wifi_connected(const char *event, const void *data, size_t len, void *ctx)
// {
//     // Cast the incoming raw data pointer to the library's wifi_connected_t structure
//     const wifi_connected_t *info = (const wifi_connected_t *)data;
    
//     // Log successful connection details including SSID, RSSI, and channel
//     ESP_LOGI(TAG, "WiFi connected to %s (RSSI: %d dBm, Channel: %d)",
//              info->ssid, info->rssi, info->channel);

//     // Upgrade the priority of the successfully connected network
//     priorityUpgrade(info->ssid);

//     // Record the connection event into persistent file storage if available
// // Record the connection event into persistent file storage without exposing passwords
//    wifi_status_t status;
//         ;
//     if (wifiConditionRecorderFile&&wifi_manager_get_status(&status) == ESP_OK) {
     

//         WifiConnectedEvent connectionEvent(info->ssid, status., info->rssi, info->channel);
//         wifiConditionRecorderFile->save(connectionEvent);
//     }

//     // Trigger cleanup of unused/unconnected network slots from NVS
//     cleanup_unconnected_networks(info->ssid);
// }

// /**
//  * @brief Helper function to upgrade/increase the priority of a connected network
//  * @param ssid Target Wi-Fi network SSID string
//  */
// void priorityUpgrade(const char *ssid)
// {
//     wifi_network_t net;
    
//     // Retrieve the existing network configuration object from NVS storage
//     if (wifi_manager_get_network(ssid, &net) == ESP_OK) {
//         // Check if the current network's priority has maxed out at 255
//         if (net.priority >= 255) {
//             net.priority = 255; 
//             ESP_LOGI(TAG, "Network '%s' reached max priority (255). Lowering other networks' priorities...", ssid);
            
//             // First, dynamically fetch count to allocate exact size without hardcoding
//             size_t count = 0;
//             if (wifi_manager_list_networks(nullptr, 0, &count) == ESP_OK && count > 0) {
//                 wifi_network_t* networks = new wifi_network_t[count];
                
//                 if (wifi_manager_list_networks(networks, count, &count) == ESP_OK) {
//                     for (size_t i = 0; i < count; ++i) {
//                         // Skip the active connected network
//                         if (std::string(networks[i].ssid) == std::string(ssid)) {
//                             continue;
//                         }
                        
//                         // Decrease priority, ensuring it doesn't drop below 0
//                         if (networks[i].priority > 0) {
//                             networks[i].priority -= 1;
//                             wifi_manager_update_network(&networks[i]);
//                         }
//                     }
//                 }
//                 delete[] networks;
//             }
            
//         } else {
//             // Increment priority normally if below 255
//             net.priority += 1;
//         }
        
//         // Save and update the current network back to NVS storage
//         if (wifi_manager_update_network(&net) == ESP_OK) {
//             ESP_LOGI(TAG, "Upgraded priority for network '%s' to %u", net.ssid, net.priority);
//         } else {
//             ESP_LOGW(TAG, "Failed to update priority for network '%s'", net.ssid);
//         }
        
//     } else {
//         ESP_LOGD(TAG, "Network '%s' not found in saved list (connected via fallback)", ssid);
//     }
// }

// /**
//  * @brief Cleans up saved networks from NVS if capacity exceeds threshold and they have never connected.
//  * @param current_ssid The currently active connected SSID (protected from deletion).
//  */
// void cleanup_unconnected_networks(const char *current_ssid)
// {
//     if (!wifiConditionRecorderFile) return;

//     size_t count = 0;
//     // Query exact number of saved networks dynamically (no hardcoded limits)
//     if (wifi_manager_list_networks(nullptr, 0, &count) != ESP_OK || count == 0) {
//         return;
//     }

//     // Check if saved network count exceeds our defined threshold limit
//     if (count > MAX_SAVED_WIFI_NETWORKS) {
//         wifi_network_t* all_networks = new wifi_network_t[count];
        
//         if (wifi_manager_list_networks(all_networks, count, &count) == ESP_OK) {
//             auto savedSize = wifiConditionRecorderFile->GetSize<WifiConnectedEvent>();

//             for (size_t j = 0; j < count; ++j) {
//                 bool connected_before = false;

//                 // Cross-reference against historical connection logs
//                 for (int k = 0; k < savedSize; ++k) {
//                     auto past_event = wifiConditionRecorderFile->fetch<WifiConnectedEvent>(k);
//                     if (std::strcmp(all_networks[j].ssid, past_event.ssid) == 0) {
//                         connected_before = true;
//                         break;
//                     }
//                 }

//                 // If saved in NVS, never connected before, and is not the active connection, remove it
//                 if (!connected_before && std::strcmp(all_networks[j].ssid, current_ssid) != 0) {
//                     ESP_LOGW(TAG, "Removing unused saved network '%s' due to NVS capacity limit", all_networks[j].ssid);
//                     wifi_manager_remove_network(all_networks[j].ssid);
//                     break; // Exit loop after one removal cycle for safety
//                 }
//             }
//         }
//         delete[] all_networks;
//     }
// }

// /**
//  * @brief Placeholder function to inspect waste or low-priority slots/data items.
//  */
// void inspect_waste_slot()
// {
//     // Implementation placeholder for future audits
// }

// } // namespace wifi






































#pragma once
#include "esp_wifi.h"
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
 * @brief Callback for WiFi connected event
 * @param event Event identifier string (e.g., "connected")
 * @param data Pointer to event-specific data structure (wifi_connected_t)
 * @param len Length of data payload
 * @param ctx User-defined context pointer
 */
inline void on_wifi_connected(const char *event, const void *data, size_t len, void *ctx)
{
    // Cast the incoming raw data pointer to the library's wifi_connected_t structure
    const wifi_connected_t *info = (const wifi_connected_t *)data;
    
    // Log successful connection details including SSID, RSSI, and channel
    ESP_LOGI(TAG, "+++++++++++++++++++++++++++++++WiFi connected event tag [%s] to SSID: %s (RSSI: %d dBm, Channel: %d)",
             event, info->ssid, info->rssi, info->channel);

 

         // Record the comprehensive status object directly into persistent file storage
        if (wifiConditionRecorderFile) {
          // Get current uptime in milliseconds using ESP-IDF timer to populate the time field
        uint32_t current_time_ms = (uint32_t)(esp_timer_get_time() / 1000);

        // Package the connection info and timestamp into our wrapper struct and save it
        wifiConditionRecorderFile->save(connected_wifi{*info, current_time_ms});
        }

  
}
// /**
//  * @brief Native ESP-IDF Callback for WiFi connected event
//  * @param arg User-defined argument passed during registration
//  * @param event_base Event base category (WIFI_EVENT)
//  * @param event_id Specific event identifier (WIFI_EVENT_STA_CONNECTED)
//  * @param event_data Pointer to native event data structure (wifi_event_sta_connected_t)
//  */
// void on_wifi_connected(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
// {
//     // 1. Cast the raw event data to the native ESP-IDF structure
//     wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;

//     // 2. Initialize your custom wifi_connected_t structure
//     wifi_connected_t info = {0};
    
//     // Safely copy the SSID string
//     memcpy(info.ssid, event->ssid, sizeof(info.ssid));
//     info.ssid[sizeof(info.ssid) - 1] = '\0'; // Ensure null-termination
    
//     // Populate the channel directly from the event payload
//     info.channel = event->channel;

//     // 3. Fetch RSSI dynamically since it isn't part of the connection event struct
//     wifi_ap_record_t ap_info;
//     if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
//         info.rssi = ap_info.rssi;
//     } else {
//         info.rssi = 0; // Fallback default if fetch fails
//     }

//     // 4. Log successful connection details
//     ESP_LOGI(TAG, "+++++++++++++++++++++++++++++++WiFi connected natively to SSID: %s (RSSI: %d dBm, Channel: %d)",
//              info.ssid, info.rssi, info.channel);

//     // 5. Record the status object into persistent storage if available
//     if (wifiConditionRecorderFile) {
//         // Get current uptime in milliseconds using ESP-IDF timer
//         uint32_t current_time_ms = (uint32_t)(esp_timer_get_time() / 1000);

//         // Package the connection info and timestamp into your wrapper struct and save it
//         wifiConditionRecorderFile->save(connected_wifi{info, current_time_ms});
//     }
// }

} // namespace wifi