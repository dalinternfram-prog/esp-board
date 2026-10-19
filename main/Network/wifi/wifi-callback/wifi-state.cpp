#include "wifi-state.hpp"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "WIFI_MANAGER";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Static member definitions
std::atomic<state> wifi_state::current_wifi_state{state::Idle};
int wifi_state::s_retry_num = 0;
EventGroupHandle_t wifi_state::s_wifi_event_group = xEventGroupCreate();

esp_event_handler_instance_t wifi_state::s_wifi_handler_instance = NULL;
esp_event_handler_instance_t wifi_state::s_ip_handler_instance = NULL;




 void wifi_state::event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data) {
    
    // 1. STA START EVENT
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        state current = current_wifi_state.load();
        
        // We definitely don't want to re-trigger if we are already connected or connecting
        if (current == state::Connected || current == state::Connecting) {
            ESP_LOGE(TAG, "Ignored STA_START: Device is already connected or connecting.");
            return;
        }

        // Otherwise, reset retry count, update state, and trigger connection
        s_retry_num = 0;
        current_wifi_state.store(state::Connecting);
        
        ESP_LOGI(TAG, "Wi-Fi STA started. Triggering initial connection...");
        esp_wifi_connect();
    } 

    // 2. DISCONNECT EVENT
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "Disconnected reason: %d", disconn->reason);
        

        // Map specific hardware failure codes immediately
        if (disconn->reason == WIFI_REASON_AUTH_FAIL) {
            current_wifi_state.store(state::Pass_error);
            ESP_LOGE(TAG, "Authentication failed: Incorrect Wi-Fi password.");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            return;
        } 
        else if (disconn->reason == WIFI_REASON_NO_AP_FOUND) {
            current_wifi_state.store(state::SSID_Error);
            ESP_LOGE(TAG, "SSID not found: Router is off or out of range.");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            return;
        } 
        else if (disconn->reason == WIFI_REASON_BEACON_TIMEOUT || disconn->reason == WIFI_REASON_AUTH_EXPIRE) {
            current_wifi_state.store(state::Signal_weak);
        }

        // Standard retry loop for transient drops
        if (s_retry_num < 10) {
            current_wifi_state.store(state::Connecting);
            s_retry_num++;
            ESP_LOGW(TAG, "Retrying Wi-Fi connection (%d/10)...", s_retry_num);
            esp_wifi_connect();
        } else {
            current_wifi_state.store(state::Max_Retries_Exceeded);
            ESP_LOGE(TAG, "Max retry limit reached. Giving up.");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } 
    
    // 3. GOT IP EVENT
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        
        s_retry_num = 0;
        
        // Transition safely to Connected state
        current_wifi_state.store(state::Connected);
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
 
esp_err_t wifi_state::register_handlers(void) {
    // 1. Register for all WIFI_EVENTs (STA_START, STA_DISCONNECTED, etc.)
    esp_err_t err = esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,          // Listen to all Wi-Fi events
        &event_handler,            // Your custom event handler function
        NULL,                      // User data pointer (not needed here)
        &s_wifi_handler_instance   // Output instance handle
    );
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Register specifically for IP_EVENT_STA_GOT_IP
    err = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,       // Listen only for the IP acquisition event
        &event_handler,            // Same event handler function
        NULL,                      // User data pointer
        &s_ip_handler_instance     // Output instance handle
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(err));
        // Cleanup previous registration if this fails
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, s_wifi_handler_instance);
        return err;
    }

    ESP_LOGI(TAG, "Wi-Fi and IP event handlers successfully registered.");
    return ESP_OK;
}