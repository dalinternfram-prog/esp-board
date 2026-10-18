#include "wifi_init.hpp"      // Include corresponding header file definitions
#include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
#include "esp_event.h"        // Include ESP-IDF system event loop header
#include "esp_netif.h"        // Include ESP-IDF network interface header
#include "esp_log.h"          // Include ESP-IDF logging library header

WifiInit::WifiInit()          // Constructor definition initializing member fields
    : m_is_initialized(false) {} // Initialize initialization state flag to false

WifiInit::~WifiInit() {       // Destructor definition for lifecycle cleanup
    if (m_is_initialized) {   // Check if the Wi-Fi driver was initialized
        esp_wifi_stop();      // Stop Wi-Fi driver execution
        esp_wifi_deinit();    // Deinitialize Wi-Fi driver resources
    }                           // End of conditional teardown block
}                               // End of destructor definition

bool WifiInit::initialize() { // Method to execute base Wi-Fi and TCP/IP stack initialization
    if (m_is_initialized) return true; // Return true immediately if already initialized

    ESP_LOGI(TAG, "Initializing base Wi-Fi network stack and hardware driver..."); // Log startup progress

    // Initialize underlying TCP/IP stack (ESP_ERR_INVALID_STATE is handled gracefully if already initialized globally)
    esp_err_t ret = esp_netif_init(); // Call ESP-IDF network interface initialization API
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // Check if netif initialization failed critically
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret)); // Log error details
        return false;           // Return failure status code
    }                           // End of netif error check block

    // Create default system event loop (ESP_ERR_INVALID_STATE handled if already created by another module)
    ret = esp_event_loop_create_default(); // Call ESP-IDF default event loop creation API
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // Check if event loop creation failed critically
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(ret)); // Log error details
        return false;           // Return failure status code
    }                           // End of event loop error check block

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Load default Wi-Fi driver configuration settings
    ret = esp_wifi_init(&cfg);  // Initialize Wi-Fi driver software stack with default parameters
    if (ret != ESP_OK) {        // Check if driver initialization failed
        ESP_LOGE(TAG, "Failed to initialize Wi-Fi driver: %s", esp_err_to_name(ret)); // Log error details
        return false;           // Return failure status code
    }                           // End of wifi driver error check block

    m_is_initialized = true;    // Mark base initialization state flag as successfully completed
    ESP_LOGI(TAG, "Base Wi-Fi module initialized successfully. (Zero credentials attached)."); // Log success info
    return true;                // Return success status code
}                               // End of initialize method