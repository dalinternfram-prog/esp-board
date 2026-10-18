// // #pragma once                  // Directive ensuring this header file is compiled only once per build

// // #include "esp_err.h"          // Include ESP-IDF error definitions header
// // #include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
// // #include <string>             // Include standard C++ string library header

// // class WifiClient {            // Declare class exclusively responsible for applying credentials and connecting


// //     private:
// //     // Simple state variables updated passively by the system event listener
// //     bool connected_flag = false;
// //     int last_disconnect_reason = 0;


// // public:                       // Public access specifier for external component interaction
// //     WifiClient();             // Constructor signature for class field initialization
// //     ~WifiClient();            // Destructor signature for clean resource teardown

// //     bool connect(const std::string& ssid, const std::string& password); // Set credentials and trigger connection
// //     void disconnect();        // Disconnect from current access point
// //     bool isConnected() const; // Check current connection status boolean flag

// // private:                      // Private access specifier for internal state variables
// //     static constexpr const char* TAG = "WifiClient"; // Logging tag constant string for console outputs


// // };                            // End of WifiClient class declaration




















// #pragma once                  // Directive ensuring this header file is compiled only once per build

// #include "esp_err.h"          // Include ESP-IDF error definitions header
// #include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
// #include "esp_event.h"        // [NEW] Include ESP-IDF system event loop header for Wi-Fi event tracking
// #include <string>             // Include standard C++ string library header

// class WifiClient {            // Declare class exclusively responsible for applying credentials and connecting

//     private:
//     // Simple state variables updated passively by the system event listener
//     bool connected_flag = false;
//     int last_disconnect_reason = 0;
//     bool auth_failed_flag = false; // [NEW] Flag indicating fast-fail due to wrong password / auth failure

// public:                       // Public access specifier for external component interaction
//     WifiClient();             // Constructor signature for class field initialization
//     ~WifiClient();            // Destructor signature for clean resource teardown

//     bool connect(const std::string& ssid, const std::string& password); // Set credentials and trigger connection
//     void disconnect();        // Disconnect from current access point
//     bool isConnected() const; // Check current connection status boolean flag

//     // [NEW] Getter to inspect the last Wi-Fi disconnect reason code
//     int getLastError() const;

//     // [NEW] Fast-fail helper to check if connection failed specifically due to bad credentials/authentication
//     bool hasAuthenticationFailed() const;

//     // [NEW] Passive event handler called by system bridge to update internal mailbox flags
//     void handleSystemEvent(esp_event_base_t event_base, int32_t event_id, void* event_data);

// private:                      // Private access specifier for internal state variables
//     static constexpr const char* TAG = "WifiClient"; // Logging tag constant string for console outputs

//     // [NEW] Static C-style event bridge function required by esp_event_handler_register
//     static void wifi_event_bridge(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    
// };                            // End of WifiClient class declaration






























#pragma once                  // Directive ensuring this header file is compiled only once per build

#include "esp_err.h"          // Include ESP-IDF error definitions header
#include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
#include "esp_event.h"        // [NEW] Include ESP-IDF system event loop header for Wi-Fi/IP event tracking
#include <string>             // Include standard C++ string library header

class WifiClient {            // Declare class exclusively responsible for applying credentials and connecting

    private:
    // Simple state variables updated passively by the system event listener mailbox pattern
    bool connected_flag = false;        // [NEW] Flag tracking active IP connection state
    int disconnect_reason = 0;          // [NEW] Raw disconnect reason code stored passively

public:                       // Public access specifier for external component interaction
    WifiClient();             // Constructor signature for class field initialization
    ~WifiClient();            // Destructor signature for clean resource teardown

    bool connect(const std::string& ssid, const std::string& password); // Set credentials and trigger connection
    void disconnect();        // Disconnect from current access point
    bool isConnected() const; // Check current connection status boolean flag

    // [NEW] Get the pure, raw disconnect reason code for polling/telemetry
    int getDisconnectReason() const {
        return disconnect_reason;
    }

    // [NEW] Passive event handler called by system bridge to update internal mailbox flags
    void handleSystemEvent(esp_event_base_t event_base, int32_t event_id, void* event_data);

private:                      // Private access specifier for internal state variables
    static constexpr const char* TAG = "WifiClient"; // Logging tag constant string for console outputs

    // [NEW] Static C-style event bridge function required by esp_event_handler_register
    static void wifi_event_bridge(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    
};                            // End of WifiClient class declaration