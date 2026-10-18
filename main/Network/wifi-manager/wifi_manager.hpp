#pragma once                  // Directive ensuring this header file is compiled only once per build

#include "esp_err.h"          // Include ESP-IDF error definitions header
#include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
#include "esp_event.h"        // Include ESP-IDF event loop header
#include "freertos/FreeRTOS.h" // Include FreeRTOS base header
#include "freertos/event_groups.h" // Include FreeRTOS event groups header
#include <string>             // Include standard C++ string library header

class WifiManager {           // Declare class managing Wi-Fi station connection and events
public:                       // Public access specifier for external component interaction
    WifiManager();            // Constructor signature for class initialization
    ~WifiManager();           // Destructor signature for lifecycle cleanup

    bool init();              // Initialize core network stack and Wi-Fi station mode driver
    bool connect(const std::string& ssid, const std::string& password, uint32_t timeout_ms = portMAX_DELAY); // Connect to target AP
    void disconnect();        // Disconnect from current Wi-Fi network
    bool isConnected() const; // Check if station is currently connected and has an IP address

private:                      // Private access specifier for internal helper variables and logic
    EventGroupHandle_t m_wifi_event_group; // FreeRTOS event group handle tracking connection states
    esp_netif_t* m_sta_netif; // Pointer tracking station network interface configuration
    int m_retry_num;          // Counter tracking reconnection attempt iterations
    bool m_is_connected;      // Boolean flag tracking active connection state

    // Static event handler routing ESP-IDF system events back to the class instance via user context
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    static constexpr uint32_t WIFI_CONNECTED_BIT = BIT0; // Event bit flag representing successful IP assignment
    static constexpr uint32_t WIFI_FAIL_BIT      = BIT1; // Event bit flag representing connection failure exhaustion
    static constexpr const char* TAG = "WifiManager";    // Logging tag constant string for console outputs
};                            // End of WifiManager class declaration