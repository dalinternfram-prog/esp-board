

#include "wifi-state/wifi-connect.hpp"
#include "wifi-state/wifi-disconnect.hpp"
#include "wifi-state/wifi-gotIp.hpp"
#include "WiFiManager.hpp"
#include <cstdint>
namespace wifi {





    


 
    // Initialize using the library's built-in multi-network, NVS, and HTTP features
    void begin(const std::string& fallbackAPName  ) {
 
        esp_err_t ret ;

    // Initialize esp_bus first (required by wifi_manager)
    ret = esp_bus_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize esp_bus: %s", esp_err_to_name(ret));
        return;
    }

   // Subscribe to WiFi events (before wifi_manager_init to catch early events)
    esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_CONNECTED), on_wifi_connected, NULL);
    esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_DISCONNECTED), on_wifi_disconnected, NULL);
    esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_GOT_IP), on_wifi_got_ip, NULL);
    //esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_VAR_CHANGED), on_var_changed, NULL);



        // 2. Configure the WiFi Manager using its native configuration struct
        wifi_manager_config_t config = {
            .default_networks = nullptr, // No hardcoded credentials; loads from NVS first!
            .default_network_count = 0,
            
            .auto_reconnect = true,
            
            // SoftAP & Captive portal settings if NVS is empty / connection fails
            .default_ap = {
                .ssid = "", // Will use fallback name below if needed
                .password = "12345678",
            },
            .enable_captive_portal = true, // Spawns the AP website automatically if all fails
            
            // Enable the built-in HTTP REST API provided by the library
            .http = {
                .enable = true,
                .api_base_path = "/api/wifi",
            },
        };

        // Copy fallback AP SSID safely
        strlcpy(config.default_ap.ssid, fallbackAPName.c_str(), sizeof(config.default_ap.ssid));

        // 3. Initialize the library (Handles NVS loading, background auto-connect, and web server)
        ESP_ERROR_CHECK(wifi_manager_init(&config));
        
        ESP_LOGI(TAG, "WiFi Manager started in background. Checking NVS credentials...");
    }

 
 
}