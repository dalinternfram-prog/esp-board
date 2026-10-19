#pragma once                  // Directive ensuring this header file is compiled only once per build

#include "esp_err.h"          // Include ESP-IDF error definitions header
#include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
#include "esp_event.h"        // [NEW] Include ESP-IDF system event loop header for Wi-Fi/IP event tracking
#include <string>             // Include standard C++ string library header
#include <atomic>             // [NEW] Include standard C++ atomic library for thread-safe multi-core state handling

class WifiClient {            // Declare class exclusively responsible for applying credentials and connecting

    private:
    // Simple state variables updated passively by the system event listener mailbox pattern
    std::atomic<bool> connected_flag{false};   // [NEW] Atomic flag tracking active IP connection state thread-safely
    std::atomic<int> disconnect_reason{0};     // [NEW] Atomic raw disconnect reason code stored passively

    // [NEW] Store registration instance handles so we can unregister them properly later
    esp_event_handler_instance_t wifi_event_instance = nullptr;
    esp_event_handler_instance_t ip_event_instance = nullptr;

    // [NEW] Passive event handler called by system bridge to update internal mailbox flags
    void handleSystemEvent(esp_event_base_t event_base, int32_t event_id, void* event_data);

    
public:                       // Public access specifier for external component interaction
    WifiClient();             // Constructor signature for class field initialization
    ~WifiClient();            // Destructor signature for clean resource teardown

    bool connect(const std::string& ssid, const std::string& password); // Set credentials and trigger connection
    void disconnect();        // Disconnect from current access point
    bool isConnected() const; // Check current connection status boolean flag

    // [NEW] Get the pure, raw disconnect reason code for polling/telemetry thread-safely
    int getDisconnectReason() const {
        return disconnect_reason.load();
    }


private:                      // Private access specifier for internal state variables
    static constexpr const char* TAG = "WifiClient"; // Logging tag constant string for console outputs

    // [NEW] Static C-style event bridge function required by esp_event_handler_instance_register
    static void wifi_event_bridge(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    
};                            // End of WifiClient class declaration