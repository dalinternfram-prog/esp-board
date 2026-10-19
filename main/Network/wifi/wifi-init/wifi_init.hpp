
#pragma once                    // Directive ensuring this header file is compiled only once per build

#include "esp_err.h"          // Include ESP-IDF error definitions header
#include "esp_wifi.h"
#include<string>
// CHANGED: Replaced the WifiInit class with a 'wifi' namespace containing functions
namespace wifi {                // Namespace grouping Wi-Fi initialization utilities
   


    bool initialize();          // Initialize network interface, event loop, and Wi-Fi driver (no credentials)
    void deinitialize();        // Stop and deinitialize Wi-Fi driver resources (replaces destructor functionality)


    /**
 * Safely switches the ESP32 Wi-Fi operational mode.
 * Handles stopping the driver, changing the mode, and restarting it.
 */
//bool switch_wifi_mode(wifi_mode_t target_mode);
 bool switch_wifi_mode(wifi_mode_t target_mode, wifi_config_t *wifi_config = nullptr, wifi_interface_t explicit_ifx = WIFI_IF_MAX) ;

}                               // End of wifi namespace declaration



  