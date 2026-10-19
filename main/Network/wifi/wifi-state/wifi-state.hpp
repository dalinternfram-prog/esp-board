#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <atomic>
namespace wifi {

// Define public or external state representation if needed, 
// or use the class methods to query status.
enum class state {
    Idle,                // Initial state or intentionally disconnected
    Connecting,          // Actively trying to associate/handshake with router
    Connected,           // Successfully online with an IP address
    Pass_error,          // Failed due to incorrect password (WIFI_REASON_AUTH_FAIL)
    SSID_Error,          // Failed because router name wasn't found (WIFI_REASON_NO_AP_FOUND)
    Signal_weak,         // Lost due to low signal / beacon timeout
    Max_Retries_Exceeded // Gave up after 10 failed attempts, needs user intervention
};

class wifi_state {

private:
    wifi_state() = default;
    ~wifi_state() = default;
    wifi_state(const wifi_state&) = delete;
    wifi_state& operator=(const wifi_state&) = delete;

    // Internal variables preserved from your design
    static std::atomic<state> current_wifi_state;
    static int s_retry_num;
    static EventGroupHandle_t s_wifi_event_group;
    
    static esp_event_handler_instance_t s_wifi_handler_instance;
    static esp_event_handler_instance_t s_ip_handler_instance;

    // The core event handler function
    static void event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data);
                              
public:
    // Singleton access or instance-based; here we expose static/instance methods matching your logic
    static wifi_state& getInstance() {
        static wifi_state instance;
        return instance;
    }

    // Register the Wi-Fi and IP event handlers
    esp_err_t register_handlers();

    // Public getter for the caller to check the current state safely
    state get_current_state() const {
        return current_wifi_state.load();
    }

    // Helper to check if currently connected
    bool is_connected() const {
        return current_wifi_state.load() == state::Connected;
    }

    // Helper to check if in a failure state requiring user intervention
    bool has_failed() const {
        state s = current_wifi_state.load();
        return s == state::Pass_error || s == state::SSID_Error || s == state::Max_Retries_Exceeded;
    }

    // Reset state manually if starting a new connection attempt or mode switch
    void reset_state() {
        current_wifi_state.store(state::Idle);
        s_retry_num = 0;
    }


};





} ;

































// /**
//  * @file main.c
//  * @brief ESP WiFi Manager - BLE Example
//  *
//  * This example demonstrates WiFi Manager with BLE GATT interface:
//  * - Configure WiFi networks via BLE from smartphone or Python CLI
//  * - Enable HTTP REST API for web-based configuration
//  * - Enable captive portal for initial setup
//  */

// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
//     #include "nvs_flash.h"
//     #include "esp_wifi_manager.h"
// #include "esp_bus.h"

// static const char *TAG = "wifi_ble_example";

// static void on_wifi_connected(const char *event, const void *data, size_t len, void *ctx)
// {
//     const wifi_connected_t *info = (const wifi_connected_t *)data;
//     ESP_LOGI(TAG, "WiFi connected to %s (RSSI: %d dBm)", info->ssid, info->rssi);
// }

// static void on_wifi_disconnected(const char *event, const void *data, size_t len, void *ctx)
// {
//     const wifi_disconnected_t *info = (const wifi_disconnected_t *)data;
//     ESP_LOGW(TAG, "WiFi disconnected from %s (reason: %d)", info->ssid, info->reason);
// }

// static void on_wifi_got_ip(const char *event, const void *data, size_t len, void *ctx)
// {
//     wifi_status_t status;
//     if (wifi_manager_get_status(&status) == ESP_OK) {
//         ESP_LOGI(TAG, "Got IP: %s", status.ip);
//     }
// }

// void app_main(void)
// {
//     // Initialize NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     ESP_LOGI(TAG, "Starting WiFi Manager with BLE example");

//     // Initialize esp_bus
//     ret = esp_bus_init();
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to initialize esp_bus: %s", esp_err_to_name(ret));
//         return;
//     }

//     // Subscribe to WiFi events
//     esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_CONNECTED), on_wifi_connected, NULL);
//     esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_DISCONNECTED), on_wifi_disconnected, NULL);
//     esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_GOT_IP), on_wifi_got_ip, NULL);

//     // Initialize WiFi Manager with BLE
//     wifi_manager_config_t config = {
//         // Retry configuration
//         .max_retry_per_network = 3,
//         .retry_interval_ms = 5000,
//         .auto_reconnect = true,

//         // SoftAP configuration (for captive portal)
//         .default_ap = {
//             .ssid = "ESP_{id}",
//             .password = "",
//             .channel = 0,
//             .max_connections = 4,
//             .ip = "192.168.4.1",
//             .netmask = "255.255.255.0",
//             .gateway = "192.168.4.1",
//             .dhcp_start = "192.168.4.2",
//             .dhcp_end = "192.168.4.20",
//         },
//         .enable_captive_portal = true,
//         .stop_ap_on_connect = true,

//         // HTTP REST API configuration
//         .http = {
//             .enable = true,
//             .httpd = NULL,
//             .api_base_path = "/api/wifi",
//             .enable_auth = false,
//         },

//         // BLE GATT configuration
//         .ble = {
//             .enable = true,
//             .device_name = NULL,  // Use Kconfig default: "ESP32-WiFi-{id}"
//         },
//     };

//     ret = wifi_manager_init(&config);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to initialize WiFi Manager: %s", esp_err_to_name(ret));
//         return;
//     }

//     ESP_LOGI(TAG, "WiFi Manager initialized with BLE");
//     ESP_LOGI(TAG, "");
//     ESP_LOGI(TAG, "Configuration options:");
//     ESP_LOGI(TAG, "  1. BLE: Use Python CLI or smartphone app");
//     ESP_LOGI(TAG, "     python wifi_ble_cli.py scan");
//     ESP_LOGI(TAG, "     python wifi_ble_cli.py add \"SSID\" \"password\"");
//     ESP_LOGI(TAG, "");
//     ESP_LOGI(TAG, "  2. Captive Portal: Connect to AP 'ESP_xxxx'");
//     ESP_LOGI(TAG, "     Then visit http://192.168.4.1/api/wifi/");
//     ESP_LOGI(TAG, "");

//     // Wait for connection
//     ESP_LOGI(TAG, "Waiting for WiFi connection...");
//     ret = wifi_manager_wait_connected(30000);

//     if (ret == ESP_OK) {
//         ESP_LOGI(TAG, "WiFi connected successfully!");
//     } else {
//         ESP_LOGW(TAG, "WiFi connection timeout - use BLE or captive portal to configure");
//     }

//     // Main loop
//     while (1) {
//         if (wifi_manager_is_connected()) {
//             wifi_status_t status;
//             if (wifi_manager_get_status(&status) == ESP_OK) {
//                 ESP_LOGI(TAG, "Connected: %s - Signal: %d%%", status.ssid, status.quality);
//             }
//         } else {
//             ESP_LOGW(TAG, "WiFi not connected - configure via BLE or captive portal");
//         }

//         vTaskDelay(pdMS_TO_TICKS(10000));
//     }
// }