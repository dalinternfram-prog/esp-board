// #include "wifi_manager.hpp"   // Include corresponding header file definitions
// #include "esp_log.h"          // Include ESP-IDF logging library header
// #include <cstring>            // Include standard C string manipulation library header

// WifiManager::WifiManager()    // Constructor definition initializing member fields
//     : m_wifi_event_group(nullptr), // Initialize event group handle to null
//       m_sta_netif(nullptr),   // Initialize network interface pointer to null
//       m_retry_num(0),         // Initialize retry counter to zero
//       m_is_connected(false) {} // Initialize connected state flag to false

// WifiManager::~WifiManager() { // Destructor definition for clean lifecycle exit
//     disconnect();             // Ensure Wi-Fi is disconnected on destruction
//     if (m_wifi_event_group) { // Check if event group handle is valid
//         vEventGroupDelete(m_wifi_event_group); // Delete FreeRTOS event group resource
//     }
// }

// bool WifiManager::init() {    // Method initializing global network stack and Wi-Fi hardware driver
//     if (m_wifi_event_group) return true; // Return true if already initialized

//     m_wifi_event_group = xEventGroupCreate(); // Create FreeRTOS event group for thread-safe synchronization
//     if (!m_wifi_event_group) { // Check if event group creation failed
//         ESP_LOGE(TAG, "Failed to create Wi-Fi event group"); // Log error message
//         return false;         // Return failure status code
//     }

//     // Initialize underlying TCP/IP stack (safe to call multiple times in ESP-IDF)
//     ESP_ERROR_CHECK(esp_netif_init());
    
//     // Create default system event loop (handled gracefully if already created elsewhere)
//     esp_event_loop_create_default();

//     m_sta_netif = esp_netif_create_default_wifi_sta(); // Create default network interface for Wi-Fi station
//     if (!m_sta_netif) {       // Check if network interface creation failed
//         ESP_LOGE(TAG, "Failed to create default Wi-Fi STA netif"); // Log error message
//         return false;         // Return failure status code
//     }

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Load default Wi-Fi driver configuration settings
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Initialize Wi-Fi driver with loaded configuration

//     // Register event handlers, passing 'this' instance pointer as user data for clean object-oriented callback routing
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::event_handler, this, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::event_handler, this, NULL));

//     ESP_LOGI(TAG, "WifiManager initialized successfully."); // Log initialization success notice
//     return true;              // Return success status code
// }

// bool WifiManager::connect(const std::string& ssid, const std::string& password, uint32_t timeout_ms) {
//     if (!m_wifi_event_group) { // Check if manager was initialized
//         ESP_LOGE(TAG, "WifiManager not initialized. Call init() first."); // Log error message
//         return false;         // Return failure status code
//     }

//     m_retry_num = 0;          // Reset retry attempt counter before starting new connection sequence
//     xEventGroupClearBits(m_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT); // Clear synchronization event bits

//     wifi_config_t wifi_config = {}; // Initialize Wi-Fi configuration structure with zeroes
//     std::strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid)); // Copy target SSID string safely
//     std::strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password)); // Copy target Password safely

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Set Wi-Fi operating mode specifically to Station (STA)
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // Apply SSID and password settings to station interface
    
//     ESP_LOGI(TAG, "Connecting to SSID: %s...", ssid.c_str()); // Log connection attempt info
//     ESP_ERROR_CHECK(esp_wifi_start()); // Start Wi-Fi driver and trigger connection sequence

//     // Disable Wi-Fi power save mode for optimal low-latency audio streaming performance
//     esp_wifi_set_ps(WIFI_PS_NONE);

//     // Block execution until either the connected bit or fail bit is set within the specified timeout duration
//     EventBits_t bits = xEventGroupWaitBits(m_wifi_event_group,
//                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                            pdFALSE, pdFALSE, timeout_ms);

//     if (bits & WIFI_CONNECTED_BIT) { // Check if connection succeeded and IP was assigned
//         ESP_LOGI(TAG, "Successfully connected to SSID: %s", ssid.c_str()); // Log success message
//         return true;          // Return true indicating successful connection
//     } else {                  // Handle connection failure or timeout case
//         ESP_LOGE(TAG, "Failed to connect to SSID: %s", ssid.c_str()); // Log failure message
//         return false;         // Return false indicating connection failed
//     }
// }

// void WifiManager::disconnect() { // Method to disconnect from Wi-Fi and stop driver execution
//     if (!m_wifi_event_group) return; // Return immediately if uninitialized

//     esp_wifi_stop();          // Stop Wi-Fi driver software stack execution
//     m_is_connected = false;   // Reset internal connection state flag
//     ESP_LOGI(TAG, "Wi-Fi disconnected and stopped."); // Log disconnection notice
// }

// bool WifiManager::isConnected() const { // Method returning active connection status boolean
//     return m_is_connected;    // Return internal connected state variable
// }

// void WifiManager::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     WifiManager* instance = static_cast<WifiManager*>(arg); // Cast generic user argument back to WifiManager instance pointer
//     if (!instance) return;    // Validate instance pointer safety

//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) { // Check if Wi-Fi station started event occurred
//         esp_wifi_connect();   // Trigger connection request
//     } 
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { // Check if disconnected event occurred
//         wifi_event_sta_disconnected_t* disconn = static_cast<wifi_event_sta_disconnected_t*>(event_data); // Cast event data
//         ESP_LOGW(TAG, "Disconnected reason: %d", disconn->reason); // Log disconnection reason code
        
//         instance->m_is_connected = false; // Mark connection status as false
        
//         if (instance->m_retry_num < 10) { // Check if retry attempts have not exceeded limit
//             esp_wifi_connect();           // Retry connection request
//             instance->m_retry_num++;      // Increment retry counter
//             ESP_LOGW(TAG, "Retrying Wi-Fi connection (%d/10)...", instance->m_retry_num); // Log retry info
//         } else {                          // Handle case where maximum retries are exhausted
//             xEventGroupSetBits(instance->m_wifi_event_group, WIFI_FAIL_BIT); // Signal failure event bit
//         }
//     } 
//     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { // Check if station successfully acquired IP address
//         ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data); // Cast IP event data payload
//         ESP_LOGI(TAG, "Got IP Address:" IPSTR, IP2STR(&event->ip_info.ip)); // Log acquired IP address format
        
//         instance->m_retry_num = 0;    // Reset retry counter on success
//         instance->m_is_connected = true; // Mark connection status as true
//         xEventGroupSetBits(instance->m_wifi_event_group, WIFI_CONNECTED_BIT); // Signal success event bit
//     }
// }