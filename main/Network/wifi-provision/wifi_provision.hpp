// #pragma once                  // Directive ensuring this header file is compiled only once per build

// #include "esp_err.h"          // Include ESP-IDF error definitions header
// #include "esp_netif.h"        // Include ESP-IDF network interface header
// #include "esp_http_server.h"  // Include ESP-IDF HTTP server component header
// #include <string>             // Include standard C++ string library header
// #include <mutex>              // Include standard C++ mutex library header for thread safety

// class WifiProvision {         // Declare class exclusively responsible for capturing and storing provisioning inputs
// public:                       // Public access specifier for external component interaction
//     WifiProvision();          // Constructor signature for class field initialization
//     ~WifiProvision();         // Destructor signature for clean resource teardown

//     bool start();             // Start SoftAP mode and HTTP provisioning web server
//     bool stop();              // Stop SoftAP mode and HTTP web server resources
    
//     bool hasCredentials() const; // Check if user has submitted and stored new credentials
//     std::string getSsid() const;     // Retrieve the stored SSID string value
//     std::string getPassword() const; // Retrieve the stored Password string value

// private:                      // Private access specifier for internal helper variables and routing
//     httpd_handle_t m_server_handle; // Handle tracking active HTTP web server instance
//     esp_netif_t* m_ap_netif;  // Pointer tracking SoftAP network interface configuration
//     bool m_is_running;        // Boolean flag tracking active provisioning server state

//     std::string m_stored_ssid;     // Internal buffer storing captured SSID string
//     std::string m_stored_password; // Internal buffer storing captured Password string
//     bool m_credentials_received;   // Boolean flag tracking whether credentials have arrived
//     mutable std::mutex m_mutex;    // Mutex ensuring thread-safe access to stored credentials across tasks

//     static esp_err_t handle_root_get(httpd_req_t *req);  // Static HTTP GET handler serving the HTML form
//     static esp_err_t handle_save_post(httpd_req_t *req); // Static HTTP POST handler parsing submitted form fields

//     static constexpr const char* TAG = "WifiProvision"; // Logging tag constant string for console outputs

 
// private:
//     static void dns_server_task(void *pvParameters);
//     TaskHandle_t m_dns_task_handle = nullptr;
//     int m_dns_socket = -1;
// };                            // End of WifiProvision class declaration


#pragma once                 // Directive ensuring this header file is compiled only once per build

#include "esp_err.h"         // Include ESP-IDF error definitions header
#include "esp_netif.h"       // Include ESP-IDF network interface header
#include "esp_http_server.h" // Include ESP-IDF HTTP server component header
#include <string>            // Include standard C++ string library header
#include <mutex>             // Include standard C++ mutex library header for thread safety

class WifiProvision {        // Declare class exclusively responsible for capturing and storing provisioning inputs
public:                      // Public access specifier for external component interaction
    WifiProvision();         // Constructor signature for class field initialization
    ~WifiProvision();        // Destructor signature for clean resource teardown

    bool start();            // Start SoftAP mode and HTTP provisioning web server
    bool stop();             // Stop SoftAP mode and HTTP web server resources
    
    bool hasCredentials() const; // Check if user has submitted and stored new credentials
    std::string getSsid() const;     // Retrieve the stored SSID string value
    std::string getPassword() const; // Retrieve the stored Password string value

private:                     // Private access specifier for internal helper variables and routing
    httpd_handle_t m_server_handle; // Handle tracking active HTTP web server instance
    esp_netif_t* m_ap_netif;  // Pointer tracking SoftAP network interface configuration
    bool m_is_running;        // Boolean flag tracking active provisioning server state

    std::string m_stored_ssid;     // Internal buffer storing captured SSID string
    std::string m_stored_password; // Internal buffer storing captured Password string
    bool m_credentials_received;    // Boolean flag tracking whether credentials have arrived
    mutable std::mutex m_mutex;    // Mutex ensuring thread-safe access to stored credentials across tasks

    static esp_err_t handle_root_get(httpd_req_t *req);  // Static HTTP GET handler serving the HTML form
    static esp_err_t handle_save_post(httpd_req_t *req); // Static HTTP POST handler parsing submitted form fields

    static constexpr const char* TAG = "WifiProvision"; // Logging tag constant string for console outputs
};                           // End of WifiProvision class declaration