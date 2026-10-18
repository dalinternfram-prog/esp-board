#pragma once                  // Directive ensuring this header file is compiled only once per build

#include "esp_err.h"          // Include ESP-IDF error definitions header

class WifiInit {              // Declare class exclusively responsible for base Wi-Fi hardware and stack initialization
public:                       // Public access specifier for external component interaction
    WifiInit();               // Constructor signature for class field initialization
    ~WifiInit();              // Destructor signature for clean resource teardown

    bool initialize();        // Initialize network interface, event loop, and Wi-Fi driver (no credentials)

private:                      // Private access specifier for internal state tracking
    bool m_is_initialized;    // Boolean flag tracking whether base initialization has completed successfully
    static constexpr const char* TAG = "WifiInit"; // Logging tag constant string for console outputs
};                            // End of WifiInit class declaration