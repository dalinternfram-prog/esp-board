// #include "wifi_manager.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
// #include <atomic>
// #include <string.h>

// static const char *TAG = "WIFI_MANAGER";

// #define WIFI_CONNECTED_BIT BIT0
// #define WIFI_FAIL_BIT      BIT1



// enum class state {
//     Idle,                // Initial state or intentionally disconnected
//     Connecting,          // Actively trying to associate/handshake with router
//     Connected,           // Successfully online with an IP address
//     Pass_error,          // Failed due to incorrect password (WIFI_REASON_AUTH_FAIL)
//     SSID_Error,          // Failed because router name wasn't found (WIFI_REASON_NO_AP_FOUND)
//     Signal_weak,         // Lost due to low signal / beacon timeout
//     Max_Retries_Exceeded // Gave up after 10 failed attempts, needs user intervention
// };
// state _my_state;

// static EventGroupHandle_t s_wifi_event_group;

// static int s_retry_num = 0;



//  // Make the state atomic for thread-safe cross-task access
// std::atomic<state> current_wifi_state{state::Idle};

//  static void event_handler(void* arg, esp_event_base_t event_base,
//                             int32_t event_id, void* event_data) {
    
//     // 1. STA START EVENT
// if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//     state current = current_wifi_state.load();
    
//     // We definitely don't want to re-trigger if we are already connected or connecting
//     if (current == state::Connected || current == state::Connecting) {
//         ESP_LOGW(TAG, "Ignored STA_START: Device is already connected or connecting.");
//         return;
//     }

//     // Otherwise, reset retry count, update state, and trigger connection
//     s_retry_num = 0;
//     current_wifi_state.store(state::Connecting);
    
//     ESP_LOGI(TAG, "Wi-Fi STA started. Triggering initial connection...");
//     esp_wifi_connect();
// }

//     // 2. DISCONNECT EVENT
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*) event_data;
//         ESP_LOGW(TAG, "Disconnected reason: %d", disconn->reason);
        
        

//         // Map specific hardware failure codes immediately
//         if (disconn->reason == WIFI_REASON_AUTH_FAIL) {
//             current_wifi_state.store(state::Pass_error);
//             ESP_LOGE(TAG, "Authentication failed: Incorrect Wi-Fi password.");
//             xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//             return;
//         } 
//         else if (disconn->reason == WIFI_REASON_NO_AP_FOUND) {
//             current_wifi_state.store(state::SSID_Error);
//             ESP_LOGE(TAG, "SSID not found: Router is off or out of range.");
//             xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//             return;
//         } 
//         else if (disconn->reason == WIFI_REASON_BEACON_TIMEOUT || disconn->reason == WIFI_REASON_AUTH_EXPIRE) {
//             current_wifi_state.store(state::Signal_weak);
//         }

//         // Standard retry loop for transient drops
//         if (s_retry_num < 10) {
//             current_wifi_state.store(state::Connecting);
//             s_retry_num++;
//             ESP_LOGW(TAG, "Retrying Wi-Fi connection (%d/10)...", s_retry_num);
//             esp_wifi_connect();
//         } else {
//             current_wifi_state.store(state::Max_Retries_Exceeded);
//             ESP_LOGE(TAG, "Max retry limit reached. Giving up.");
//             xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//         }
//     } 
    
//     // 3. GOT IP EVENT
//     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI(TAG, "Got IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        
//         s_retry_num = 0;
        
//         // Transition safely to Connected state
//         current_wifi_state.store(state::Connected);
        
//         xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//     }
// }



// // Keep track of handler instances if you ever need to unregister them
// static esp_event_handler_instance_t s_wifi_handler_instance = NULL;
// static esp_event_handler_instance_t s_ip_handler_instance = NULL;

// esp_err_t register_wifi_event_handler(void) {
//     // 1. Register for all WIFI_EVENTs (STA_START, STA_DISCONNECTED, etc.)
//     esp_err_t err = esp_event_handler_instance_register(
//         WIFI_EVENT,
//         ESP_EVENT_ANY_ID,          // Listen to all Wi-Fi events
//         &event_handler,            // Your custom event handler function
//         NULL,                      // User data pointer (not needed here)
//         &s_wifi_handler_instance   // Output instance handle
//     );
    
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(err));
//         return err;
//     }

//     // 2. Register specifically for IP_EVENT_STA_GOT_IP
//     err = esp_event_handler_instance_register(
//         IP_EVENT,
//         IP_EVENT_STA_GOT_IP,       // Listen only for the IP acquisition event
//         &event_handler,            // Same event handler function
//         NULL,                      // User data pointer
//         &s_ip_handler_instance     // Output instance handle
//     );

//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(err));
//         // Cleanup previous registration if this fails
//         esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, s_wifi_handler_instance);
//         return err;
//     }

//     ESP_LOGI(TAG, "Wi-Fi and IP event handlers successfully registered.");
//     return ESP_OK;
// }





