#pragma once                  // Directive ensuring this header file is compiled only once per build

#include "esp_err.h"          // Include ESP-IDF error definitions header
#include "esp_wifi.h"         // Include ESP-IDF Wi-Fi driver header
#include <string>             // Include standard C++ string library header

class WifiClient {            // Declare class exclusively responsible for applying credentials and connecting
public:                       // Public access specifier for external component interaction
    WifiClient();             // Constructor signature for class field initialization
    ~WifiClient();            // Destructor signature for clean resource teardown

    bool connect(const std::string& ssid, const std::string& password); // Set credentials and trigger connection
    void disconnect();        // Disconnect from current access point
    bool isConnected() const; // Check current connection status boolean flag

private:                      // Private access specifier for internal state variables
    static constexpr const char* TAG = "WifiClient"; // Logging tag constant string for console outputs
};                            // End of WifiClient class declaration