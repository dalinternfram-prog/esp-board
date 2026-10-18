// #include "wifi_provision.hpp" // Include corresponding header file definitions
// #include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
// #include "esp_event.h"        // Include ESP-IDF system event loop header
// #include "esp_log.h"          // Include ESP-IDF logging library header
// #include <cstring>            // Include standard C string manipulation library header
// #include <algorithm>          // Include standard C++ algorithm library header

// WifiProvision::WifiProvision() // Constructor definition initializing member fields
//     : m_server_handle(nullptr), // Initialize HTTP server handle pointer to null
//       m_ap_netif(nullptr),      // Initialize AP network interface pointer to null
//       m_is_running(false),      // Initialize active running state flag to false
//       m_credentials_received(false) {} // Initialize credentials received flag to false

// WifiProvision::~WifiProvision() { // Destructor definition for clean lifecycle exit
//     stop();                   // Ensure provisioning server and AP are shut down on destruction
// }                               // End of destructor definition


// // DNS Server Task (UDP Port 53) - Intercepts all DNS requests and points them to 192.168.4.1
// void WifiProvision::dns_server_task(void *pvParameters) {
//     char rx_buffer[512];
//     char tx_buffer[512];
//     struct sockaddr_in dest_addr;
//     dest_addr.sin_family = AF_INET;
//     dest_addr.sin_port = htons(53);
//     dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);

//     int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//     if (sock < 0) {
//         ESP_LOGE("DNS", "Unable to create DNS socket: errno %d", errno);
//         vTaskDelete(NULL);
//         return;
//     }

//     int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
//     if (err < 0) {
//         ESP_LOGE("DNS", "Socket unable to bind: errno %d", errno);
//         close(sock);
//         vTaskDelete(NULL);
//         return;
//     }

//     ESP_LOGI("DNS", "Captive Portal DNS Server running on port 53");

//     while (true) {
//         struct sockaddr_in source_addr;
//         socklen_t socklen = sizeof(source_addr);
//         int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

//         if (len < 0) break;

//         // Simple DNS hijacking response builder
//         if (len > 12) {
//             std::memcpy(tx_buffer, rx_buffer, len);
//             tx_buffer[2] = 0x81; // Response flags
//             tx_buffer[3] = 0x80;
//             tx_buffer[6] = 0x00; tx_buffer[7] = 0x01; // Answer count = 1
//             tx_buffer[8] = 0x00; tx_buffer[9] = 0x00;
//             tx_buffer[10] = 0x00; tx_buffer[11] = 0x00;

//             int reply_len = len;
//             // Append DNS answer pointing to 192.168.4.1
//             const unsigned char res_data[] = {
//                 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 
//                 0x00, 0x3c, 0x00, 0x04, 192, 168, 4, 1
//             };
//             if (reply_len + sizeof(res_data) < sizeof(tx_buffer)) {
//                 std::memcpy(tx_buffer + reply_len, res_data, sizeof(res_data));
//                 reply_len += sizeof(res_data);
//                 sendto(sock, tx_buffer, reply_len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
//             }
//         }
//     }
//     close(sock);
//     vTaskDelete(NULL);
// }

// // Catch-all handler to redirect any unrecognized URL to the root configuration page
// esp_err_t handle_redirect_get(httpd_req_t *req) {
//     httpd_resp_set_status(req, "302 Found");
//     httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
//     httpd_resp_send(req, nullptr, 0);
//     return ESP_OK;
// }



// bool WifiProvision::start() { // Method to start SoftAP and provisioning web server
//     if (m_is_running) return true; // Return true immediately if already running

//     ESP_LOGI(TAG, "Starting Provisioning SoftAP and Web Server..."); // Log startup progress info

//     m_ap_netif = esp_netif_create_default_wifi_ap(); // Create default network interface for Access Point
//     if (!m_ap_netif) {        // Check if network interface creation failed
//         ESP_LOGE(TAG, "Failed to create default Wi-Fi AP network interface"); // Log error details
//         return false;         // Return failure status code
//     }                           // End of netif check block

//     wifi_config_t wifi_config = {}; // Initialize Wi-Fi configuration structure with zeroes
//     std::strcpy((char*)wifi_config.ap.ssid, "ESP32_Config_AP"); // Copy default configuration SSID string
//     wifi_config.ap.ssid_len = std::strlen("ESP32_Config_AP"); // Set SSID length value explicitly
//     wifi_config.ap.channel = 1; // Set Wi-Fi radio channel to 1
//     wifi_config.ap.max_connection = 4; // Set maximum simultaneous client connection limit to 4
//     wifi_config.ap.authmode = WIFI_AUTH_OPEN; // Set authentication mode to open (no password for config AP)

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // Configure Wi-Fi driver operating mode to Access Point
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config)); // Apply AP configuration settings to driver

//     ESP_ERROR_CHECK(esp_wifi_start()); // Start Wi-Fi driver execution in AP mode
// // Start UDP DNS Server task for captive portal detection
//     xTaskCreate(dns_server_task, "dns_server", 3072, NULL, 4, &m_dns_task_handle);


//     httpd_config_t config = HTTPD_DEFAULT_CONFIG(); // Load default HTTP server configuration settings struct
//     if (httpd_start(&m_server_handle, &config) == ESP_OK) { // Attempt to start HTTP web server instance
//         httpd_uri_t root_uri = { // Create URI handler structure for root GET request
//             .uri      = "/",    // Target root path endpoint string
//             .method   = HTTP_GET, // HTTP request method type set to GET
//             .handler  = handle_root_get, // Callback function pointer handling root requests
//             .user_ctx = this     // Pass pointer to current class instance as context
//         };                       // End of root URI struct initialization
//         httpd_register_uri_handler(m_server_handle, &root_uri); // Register root GET URI handler with web server

//         httpd_uri_t save_uri = { // Create URI handler structure for saving form data via POST
//             .uri      = "/save", // Target save path endpoint string
//             .method   = HTTP_POST, // HTTP request method type set to POST
//             .handler  = handle_save_post, // Callback function pointer handling save requests
//             .user_ctx = this     // Pass pointer to current class instance as context
//         };                       // End of save URI struct initialization
//         httpd_register_uri_handler(m_server_handle, &save_uri); // Register save POST URI handler with web server



// // Wildcard catch-all handler for automatic captive portal redirection
//         httpd_uri_t wildcard_uri = { "/*", HTTP_GET, handle_redirect_get, this };
//         httpd_register_uri_handler(m_server_handle, &wildcard_uri);





//     } else {                    // Handle HTTP server startup failure case
//         ESP_LOGE(TAG, "Failed to start HTTP provisioning server"); // Log error details
//         return false;           // Return failure status code
//     }                           // End of HTTP server startup conditional block

//     m_is_running = true;        // Mark active running state flag as true
//     ESP_LOGI(TAG, "Provisioning active. Connect to 'ESP32_Config_AP' and open browser at 192.168.4.1"); // Log instructions
//     return true;                // Return success status code
// }                               // End of start method

// bool WifiProvision::stop() {    // Method to stop SoftAP mode and web server resources
//     if (!m_is_running) return true; // Return true if already stopped

//      if (m_dns_task_handle) {
//         vTaskDelete(m_dns_task_handle);
//         m_dns_task_handle = nullptr;
//     }

//     if (m_server_handle) {      // Check if HTTP server handle pointer is valid
//         httpd_stop(m_server_handle); // Stop and destroy active HTTP web server instance
//         m_server_handle = nullptr; // Reset server handle pointer variable to null
//     }                           // End of server stop block

//     esp_wifi_stop();            // Stop Wi-Fi driver software stack execution
    
//     if (m_ap_netif) {           // Check if AP network interface pointer is valid
//         esp_netif_destroy(m_ap_netif); // Destroy network interface instance
//         m_ap_netif = nullptr;   // Reset network interface pointer to null
//     }                           // End of netif destruction block

//     m_is_running = false;       // Reset active running state flag to false
//     ESP_LOGI(TAG, "WifiProvision stopped successfully."); // Log success message
//     return true;                // Return success status code
// }                               // End of stop method

// bool WifiProvision::hasCredentials() const { // Method to check if credentials have been received asynchronously
//     std::lock_guard<std::mutex> lock(m_mutex); // Lock mutex to ensure thread safety across tasks
//     return m_credentials_received;             // Return boolean flag indicating if data arrived
// }                               // End of hasCredentials method

// std::string WifiProvision::getSsid() const {    // Method to retrieve stored SSID string
//     std::lock_guard<std::mutex> lock(m_mutex); // Lock mutex for thread-safe read access
//     return m_stored_ssid;                      // Return copy of stored SSID string
// }                               // End of getSsid method

// std::string WifiProvision::getPassword() const { // Method to retrieve stored Password string
//     std::lock_guard<std::mutex> lock(m_mutex); // Lock mutex for thread-safe read access
//     return m_stored_password;                  // Return copy of stored Password string
// }                               // End of getPassword method

// esp_err_t WifiProvision::handle_root_get(httpd_req_t *req) { // Static callback serving HTML configuration form page
//     const char* html_page = 
//         "<html><head><title>ESP32 Setup</title></head>"
//         "<body style='font-family: Arial; text-align: center; margin-top: 50px;'>"
//         "<h2>Wi-Fi Provisioning</h2>"
//         "<form action='/save' method='POST'>"
//         "<input type='text' name='ssid' placeholder='SSID' required style='padding: 10px; width: 250px; margin: 10px;'><br>"
//         "<input type='password' name='password' placeholder='Password' required style='padding: 10px; width: 250px; margin: 10px;'><br>"
//         "<button type='submit' style='padding: 10px 20px; background: #28a745; color: white; border: none; border-radius: 5px;'>Connect</button>"
//         "</form></body></html>"; // HTML string containing simple web form layout
    
//     httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN); // Send HTML response string back to connected client browser
//     return ESP_OK;              // Return success error code indicating request handled successfully
// }                               // End of handle_root_get method

// esp_err_t WifiProvision::handle_save_post(httpd_req_t *req) { // Static callback processing submitted form data asynchronously
//     char content[100] = {};     // Initialize local character buffer array for incoming request payload content
//     size_t recv_size = std::min(req->content_len, sizeof(content) - 1); // Calculate safe maximum bytes to receive

//     int ret = httpd_req_recv(req, content, recv_size); // Receive HTTP POST body payload data from client request
//     if (ret <= 0) {             // Check if data reception failed or payload was empty
//         httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive payload"); // Send error response
//         return ESP_FAIL;        // Return failure status code
//     }                           // End of payload reception check block

//     char ssid[32] = {};         // Initialize character array buffer for extracted SSID string
//     char password[64] = {};     // Initialize character array buffer for extracted Password string
    
//     if (sscanf(content, "ssid=%31[^&]&password=%63s", ssid, password) == 2) { // Verify form parameter parsing success
//         WifiProvision* instance = static_cast<WifiProvision*>(req->user_ctx); // Cast user context pointer back to class instance
//         if (instance) {         // Check if instance pointer is valid
//             std::lock_guard<std::mutex> lock(instance->m_mutex); // Lock mutex to safely update member variables across tasks
//             instance->m_stored_ssid = ssid;         // Store parsed SSID string internally
//             instance->m_stored_password = password; // Store parsed Password string internally
//             instance->m_credentials_received = true; // Mark credentials received flag as true
//         }                       // End of instance validation block

//         const char* success_resp = "<html><body><h2>Credentials Received! Device is processing...</h2></body></html>"; // Success message
//         httpd_resp_send(req, success_resp, HTTPD_RESP_USE_STRLEN); // Send success response page to client browser
//     } else {                    // Handle parameter parsing error case
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Form Parameters"); // Send HTTP 400 bad request error
//         return ESP_FAIL;        // Return failure error code
//     }                           // End of parsing check block

//     return ESP_OK;              // Return success status code
// }                               // End of handle_save_post method















#include "wifi_provision.hpp" // Include corresponding header file definitions
#include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
#include "esp_event.h"        // Include ESP-IDF system event loop header
#include "esp_log.h"          // Include ESP-IDF logging library header
#include <cstring>            // Include standard C string manipulation library header
#include <algorithm>          // Include standard C++ algorithm library header
#include "captive_portal.h"   // [NEW] Include external open-source captive portal component header for automatic OS probe redirection

WifiProvision::WifiProvision() // Constructor definition initializing member fields
    : m_server_handle(nullptr), // Initialize HTTP server handle pointer to null
      m_ap_netif(nullptr),      // Initialize AP network interface pointer to null
      m_is_running(false),      // Initialize active running state flag to false
      m_credentials_received(false) {} // Initialize credentials received flag to false

WifiProvision::~WifiProvision() { // Destructor definition for clean lifecycle exit
    stop();                     // Ensure provisioning server and AP are shut down on destruction
}                               // End of destructor definition

bool WifiProvision::start() { // Method to start SoftAP and provisioning web server
    if (m_is_running) return true; // Return true immediately if already running

    ESP_LOGI(TAG, "Starting Provisioning SoftAP and Web Server..."); // Log startup progress info

    m_ap_netif = esp_netif_create_default_wifi_ap(); // Create default network interface for Access Point
    if (!m_ap_netif) {          // Check if network interface creation failed
        ESP_LOGE(TAG, "Failed to create default Wi-Fi AP network interface"); // Log error details
        return false;           // Return failure status code
    }                           // End of netif check block

    wifi_config_t wifi_config = {}; // Initialize Wi-Fi configuration structure with zeroes
    std::strcpy((char*)wifi_config.ap.ssid, "ESP32_Config_AP"); // Copy default configuration SSID string
    wifi_config.ap.ssid_len = std::strlen("ESP32_Config_AP"); // Set SSID length value explicitly
    wifi_config.ap.channel = 1; // Set Wi-Fi radio channel to 1
    wifi_config.ap.max_connection = 4; // Set maximum simultaneous client connection limit to 4
    wifi_config.ap.authmode = WIFI_AUTH_OPEN; // Set authentication mode to open (no password for config AP)

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // Configure Wi-Fi driver operating mode to Access Point
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config)); // Apply AP configuration settings to driver
    ESP_ERROR_CHECK(esp_wifi_start()); // Start Wi-Fi driver execution in AP mode

    httpd_config_t config = HTTPD_DEFAULT_CONFIG(); // Load default HTTP server configuration settings struct
    
    // [NEW] Enable wildcard URI matching required by the captive portal component to intercept OS probes
    config.uri_match_fn = httpd_uri_match_wildcard; 
// [NEW] Increase the internal receive buffer size to accommodate large mobile browser request headers
    config.max_uri_len = 1024;        // Allow longer URIs
    config.stack_size = 4096;         // Give the httpd worker task enough stack space
    config.ctrl_port = 32768;         // Default control port

    // Note: ESP-IDF httpd uses the rx_buf size to handle incoming request headers. 
    // We can override the default receive buffer size by setting it in config:
    config.recv_wait_timeout = 5;
    config.send_wait_timeout = 5;


    if (httpd_start(&m_server_handle, &config) == ESP_OK) { // Attempt to start HTTP web server instance
        httpd_uri_t root_uri = { // Create URI handler structure for root GET request
            .uri      = "/",    // Target root path endpoint string
            .method   = HTTP_GET, // HTTP request method type set to GET
            .handler  = handle_root_get, // Callback function pointer handling root requests
            .user_ctx = this     // Pass pointer to current class instance as context
        };                       // End of root URI struct initialization
        httpd_register_uri_handler(m_server_handle, &root_uri); // Register root GET URI handler with web server

        httpd_uri_t save_uri = { // Create URI handler structure for saving form data via POST
            .uri      = "/save", // Target save path endpoint string
            .method   = HTTP_POST, // HTTP request method type set to POST
            .handler  = handle_save_post, // Callback function pointer handling save requests
            .user_ctx = this     // Pass pointer to current class instance as context
        };                       // End of save URI struct initialization
        httpd_register_uri_handler(m_server_handle, &save_uri); // Register save POST URI handler with web server

        // [NEW] Register the external captive portal component to automatically handle DNS hijacking and mobile popup redirects
        captive_portal_register(m_server_handle, NULL);

    } else {                    // Handle HTTP server startup failure case
        ESP_LOGE(TAG, "Failed to start HTTP provisioning server"); // Log error details
        return false;           // Return failure status code
    }                           // End of HTTP server startup conditional block

    m_is_running = true;        // Mark active running state flag as true
    ESP_LOGI(TAG, "Provisioning active. Connect to 'ESP32_Config_AP' and open browser at 192.168.4.1"); // Log instructions
    return true;                // Return success status code
}                               // End of start method

bool WifiProvision::stop() {    // Method to stop SoftAP mode and web server resources
    if (!m_is_running) return true; // Return true if already stopped

    if (m_server_handle) {      // Check if HTTP server handle pointer is valid
        httpd_stop(m_server_handle); // Stop and destroy active HTTP web server instance
        m_server_handle = nullptr; // Reset server handle pointer variable to null
    }                           // End of server stop block

    esp_wifi_stop();            // Stop Wi-Fi driver software stack execution
    
    if (m_ap_netif) {           // Check if AP network interface pointer is valid
        esp_netif_destroy(m_ap_netif); // Destroy network interface instance
        m_ap_netif = nullptr;   // Reset network interface pointer to null
    }                           // End of netif destruction block

    m_is_running = false;       // Reset active running state flag to false
    ESP_LOGI(TAG, "WifiProvision stopped successfully."); // Log success message
    return true;                // Return success status code
}                               // End of stop method

bool WifiProvision::hasCredentials() const { // Method to check if credentials have been received asynchronously
    std::lock_guard<std::mutex> lock(m_mutex); // Lock mutex to ensure thread safety across tasks
    return m_credentials_received;             // Return boolean flag indicating if data arrived
}                               // End of hasCredentials method
    void WifiProvision::clearCredentials(){
    std::lock_guard<std::mutex> lock(m_mutex); // Lock mutex to ensure thread safety across tasks
      m_credentials_received=false;             // Return boolean flag indicating if data arrived
    }



std::string WifiProvision::getSsid() const {    // Method to retrieve stored SSID string
    std::lock_guard<std::mutex> lock(m_mutex); // Lock mutex for thread-safe read access
    return m_stored_ssid;                      // Return copy of stored SSID string
}                               // End of getSsid method

std::string WifiProvision::getPassword() const { // Method to retrieve stored Password string
    std::lock_guard<std::mutex> lock(m_mutex); // Lock mutex for thread-safe read access
    return m_stored_password;                  // Return copy of stored Password string
}                               // End of getPassword method

esp_err_t WifiProvision::handle_root_get(httpd_req_t *req) { // Static callback serving HTML configuration form page
    const char* html_page = 
        "<html><head><title>ESP32 Setup</title></head>"
        "<body style='font-family: Arial; text-align: center; margin-top: 50px;'>"
        "<h2>Wi-Fi Provisioning</h2>"
        "<form action='/save' method='POST'>"
        "<input type='text' name='ssid' placeholder='SSID' required style='padding: 10px; width: 250px; margin: 10px;'><br>"
        "<input type='password' name='password' placeholder='Password' required style='padding: 10px; width: 250px; margin: 10px;'><br>"
        "<button type='submit' style='padding: 10px 20px; background: #28a745; color: white; border: none; border-radius: 5px;'>Connect</button>"
        "</form></body></html>"; // HTML string containing simple web form layout
    
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN); // Send HTML response string back to connected client browser
    return ESP_OK;              // Return success error code indicating request handled successfully
}                               // End of handle_root_get method

esp_err_t WifiProvision::handle_save_post(httpd_req_t *req) { // Static callback processing submitted form data asynchronously
    char content[100] = {};     // Initialize local character buffer array for incoming request payload content
    size_t recv_size = std::min(req->content_len, sizeof(content) - 1); // Calculate safe maximum bytes to receive

    int ret = httpd_req_recv(req, content, recv_size); // Receive HTTP POST body payload data from client request
    if (ret <= 0) {             // Check if data reception failed or payload was empty
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive payload"); // Send error response
        return ESP_FAIL;        // Return failure status code
    }                           // End of payload reception check block

    char ssid[32] = {};         // Initialize character array buffer for extracted SSID string
    char password[64] = {};     // Initialize character array buffer for extracted Password string
    
    if (sscanf(content, "ssid=%31[^&]&password=%63s", ssid, password) == 2) { // Verify form parameter parsing success
        WifiProvision* instance = static_cast<WifiProvision*>(req->user_ctx); // Cast user context pointer back to class instance
        if (instance) {         // Check if instance pointer is valid
            std::lock_guard<std::mutex> lock(instance->m_mutex); // Lock mutex to safely update member variables across tasks
            instance->m_stored_ssid = ssid;           // Store parsed SSID string internally
            instance->m_stored_password = password; // Store parsed Password string internally
            instance->m_credentials_received = true; // Mark credentials received flag as true
        }                       // End of instance validation block

        const char* success_resp = "<html><body><h2>Credentials Received! Device is processing...</h2></body></html>"; // Success message
        httpd_resp_send(req, success_resp, HTTPD_RESP_USE_STRLEN); // Send success response page to client browser
    } else {                    // Handle parameter parsing error case
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Form Parameters"); // Send HTTP 400 bad request error
        return ESP_FAIL;        // Return failure error code
    }                           // End of parsing check block

    return ESP_OK;              // Return success status code
}                               // End of handle_save_post method