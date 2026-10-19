

// #include "wifi_init.hpp"      // Include corresponding header file definitions
// #include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
// #include "esp_event.h"        // Include ESP-IDF system event loop header
// #include "esp_netif.h"        // Include ESP-IDF network interface header
// #include "esp_log.h"          // Include ESP-IDF logging library header

// // CHANGED: Added an anonymous namespace for state tracking and logging tag, replacing class fields
// namespace {
//     bool s_is_initialized = false;            // Boolean flag tracking whether base initialization has completed successfully
//     constexpr const char* TAG = "WifiInit";   // Logging tag constant string for console outputs
// }

// namespace wifi {                              // Namespace grouping Wi-Fi initialization utilities

//     bool initialize() {                       // CHANGED: Converted class method into a standalone namespace function
//         if (s_is_initialized) return true;    // Return true immediately if already initialized (CHANGED: using s_is_initialized)

//         ESP_LOGI(TAG, "Initializing base Wi-Fi network stack and hardware driver..."); // Log startup progress

//         // Initialize underlying TCP/IP stack (ESP_ERR_INVALID_STATE is handled gracefully if already initialized globally)
//         esp_err_t ret = esp_netif_init(); // Call ESP-IDF network interface initialization API
//         if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // Check if netif initialization failed critically
//             ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret)); // Log error details
//             return false;                     // Return failure status code
//         }                                     // End of netif error check block

//         // Create default system event loop (ESP_ERR_INVALID_STATE handled if already created by another module)
//         ret = esp_event_loop_create_default(); // Call ESP-IDF default event loop creation API
//         if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // Check if event loop creation failed critically
//             ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(ret)); // Log error details
//             return false;                     // Return failure status code
//         }                                     // End of event loop error check block

//         wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Load default Wi-Fi driver configuration settings
//         ret = esp_wifi_init(&cfg);  // Initialize Wi-Fi driver software stack with default parameters
//         if (ret != ESP_OK) {        // Check if driver initialization failed
//             ESP_LOGE(TAG, "Failed to initialize Wi-Fi driver: %s", esp_err_to_name(ret)); // Log error details
//             return false;                     // Return failure status code
//         }                                     // End of wifi driver error check block

//         s_is_initialized = true;              // Mark base initialization state flag as successfully completed (CHANGED: using s_is_initialized)
//         ESP_LOGI(TAG, "Base Wi-Fi module initialized successfully. (Zero credentials attached)."); // Log success info
//         return true;                          // Return success status code
//     }                                         // End of initialize function

//     // CHANGED: Replaced the class destructor with a standalone deinitialize function
//     void deinitialize() {                     // Function for clean resource teardown
//         if (s_is_initialized) {               // Check if the Wi-Fi driver was initialized (CHANGED: using s_is_initialized)
//             esp_wifi_stop();                  // Stop Wi-Fi driver execution
//             esp_wifi_deinit();                // Deinitialize Wi-Fi driver resources
//             s_is_initialized = false;         // Reset state flag
//         }                                     // End of conditional teardown block
//     }                                         // End of deinitialize function





//     /**
//  * Safely switches the ESP32 Wi-Fi operational mode.
//  * Handles stopping the driver, changing the mode, and restarting it.
//  */
// // bool switch_wifi_mode(wifi_mode_t target_mode) {
    
// //     // Variable to hold the current operational mode of the ESP32
// //     wifi_mode_t current_mode;
    
// //     // Query the Wi-Fi driver to find out what mode it is currently running in
// //     if (esp_wifi_get_mode(&current_mode) != ESP_OK) {
// //         ESP_LOGE(TAG, "Failed to get current Wi-Fi mode");
// //         return false; // Abort if we cannot read the hardware state
// //     }

// //     // Check if the ESP32 is already running in the desired mode
// //     if (current_mode == target_mode) {
// //         ESP_LOGI(TAG, "Already in the requested Wi-Fi mode, no change needed.");
// //         return true; // Exit early since nothing needs to change
// //     }

// //     ESP_LOGI(TAG, "Switching Wi-Fi mode... Stopping current driver state.");

// //     // Stop the Wi-Fi driver before changing its configuration (mandatory in ESP-IDF)
// //     if (esp_wifi_stop() != ESP_OK) {
// //         ESP_LOGW(TAG, "Warning: Wi-Fi was not running or failed to stop cleanly.");
// //     }

// //     // Apply the new target mode (e.g., WIFI_MODE_STA or WIFI_MODE_AP) to the driver
// //     if (esp_wifi_set_mode(target_mode) != ESP_OK) {
// //         ESP_LOGE(TAG, "Failed to set the new Wi-Fi mode in the driver.");
// //         return false; // Abort if the driver rejects the mode
// //     }

// //     // Restart the Wi-Fi driver so it initializes buffers and hardware for the new mode
// //     if (esp_wifi_start() != ESP_OK) {
// //         ESP_LOGE(TAG, "Failed to restart Wi-Fi after mode change.");
// //         return false; // Abort if the driver fails to boot back up
// //     }

// //     ESP_LOGI(TAG, "Wi-Fi mode successfully changed and driver restarted.");
// //     return true; // Mode switch complete and successful
// // }

//  bool switch_wifi_mode(wifi_mode_t target_mode, wifi_config_t *wifi_config , wifi_interface_t explicit_ifx ) {
    
//     // Variable to hold the current operational mode of the ESP32
//     wifi_mode_t current_mode;
    
//     // Query the Wi-Fi driver to find out what mode it is currently running in
//     if (esp_wifi_get_mode(&current_mode) != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to get current Wi-Fi mode");
//         return false; // Abort if we cannot read the hardware state
//     }

//     // Check if the ESP32 is already running in the desired mode
//     if (current_mode == target_mode && wifi_config == nullptr) {
//         ESP_LOGI(TAG, "Already in the requested Wi-Fi mode, no change needed.");
//         return true; // Exit early since nothing needs to change
//     }

//     ESP_LOGI(TAG, "Switching Wi-Fi mode... Stopping current driver state.");

//     // Stop the Wi-Fi driver before changing its configuration (mandatory in ESP-IDF)
//     if (esp_wifi_stop() != ESP_OK) {
//         ESP_LOGW(TAG, "Warning: Wi-Fi was not running or failed to stop cleanly.");
//     }

//     // Apply the new target mode (e.g., WIFI_MODE_STA or WIFI_MODE_AP) to the driver
//     if (esp_wifi_set_mode(target_mode) != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to set the new Wi-Fi mode in the driver.");
//         return false; // Abort if the driver rejects the mode
//     }

//     // Apply configuration safely while the driver is stopped (if provided)
//     if (wifi_config != nullptr) {
//         wifi_interface_t target_ifx = explicit_ifx;

//         // If the user didn't specify an interface manually, infer it from the mode
//         if (target_ifx == WIFI_IF_MAX) {
//             if (target_mode == WIFI_MODE_AP) {
//                 target_ifx = WIFI_IF_AP;
//             } else if (target_mode == WIFI_MODE_STA) {
//                 target_ifx = WIFI_IF_STA;
//             } else {
//                 ESP_LOGE(TAG, "APSTA mode requires an explicit interface specification!");
//                 return false;
//             }
//         }

//         if (esp_wifi_set_config(target_ifx, wifi_config) != ESP_OK) {
//             ESP_LOGE(TAG, "Failed to apply Wi-Fi configuration settings.");
//             return false;
//         }
//     }

//     // Restart the Wi-Fi driver so it initializes buffers and hardware for the new mode
//     if (esp_wifi_start() != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to restart Wi-Fi after mode change.");
//         return false; // Abort if the driver fails to boot back up
//     }

//     ESP_LOGI(TAG, "Wi-Fi mode successfully changed and driver restarted.");
//     return true; // Mode switch complete and successful
// }







// }                                             // End of wifi namespace implementation