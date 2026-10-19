#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <atomic>

// Define public or external state representation if needed, 
// or use the class methods to query status.
enum class state {
    Idle,                // Initial state or intentionally disconnected
    Connecting,          // Actively trying to associate/handshake with router
    Connected,           // Successfully online with an IP address
    Pass_error,          // Failed due to incorrect password (WIFI_REASON_AUTH_FAIL)
    SSID_Error,          // Failed because router name wasn't found (WIFI_REASON_NO_AP_FOUND)
    Signal_weak,         // Lost due to low signal / beacon timeout
    Max_Retries_Exceeded // Gave up after 10 failed attempts, needs user intervention
};

class wifi_state {

private:
    wifi_state() = default;
    ~wifi_state() = default;
    wifi_state(const wifi_state&) = delete;
    wifi_state& operator=(const wifi_state&) = delete;

    // Internal variables preserved from your design
    static std::atomic<state> current_wifi_state;
    static int s_retry_num;
    static EventGroupHandle_t s_wifi_event_group;
    
    static esp_event_handler_instance_t s_wifi_handler_instance;
    static esp_event_handler_instance_t s_ip_handler_instance;

    // The core event handler function
    static void event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data);
                              
public:
    // Singleton access or instance-based; here we expose static/instance methods matching your logic
    static wifi_state& getInstance() {
        static wifi_state instance;
        return instance;
    }

    // Register the Wi-Fi and IP event handlers
    esp_err_t register_handlers();

    // Public getter for the caller to check the current state safely
    state get_current_state() const {
        return current_wifi_state.load();
    }

    // Helper to check if currently connected
    bool is_connected() const {
        return current_wifi_state.load() == state::Connected;
    }

    // Helper to check if in a failure state requiring user intervention
    bool has_failed() const {
        state s = current_wifi_state.load();
        return s == state::Pass_error || s == state::SSID_Error || s == state::Max_Retries_Exceeded;
    }

    // Reset state manually if starting a new connection attempt or mode switch
    void reset_state() {
        current_wifi_state.store(state::Idle);
        s_retry_num = 0;
    }


};