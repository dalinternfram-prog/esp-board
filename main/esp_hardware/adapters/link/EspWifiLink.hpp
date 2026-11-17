#pragma once

// Adapter: WiFi connectivity through the existing WiFi manager (ILink port).
// "Up" means associated AND holding a real IP (DHCP completed), so the session
// handshake cannot start before the stack is actually usable.
// Swap this for a Cellular4GLink / EthernetLink later without touching the core.

#include "interface.hpp"

#include <cstring>

#include "esp_timer.h"
#include "WiFiManager.hpp"
#include "wifi/shared.hpp"

namespace adapters {

class EspWifiLink : public core::ILink {
public:
    bool start() override {
        wifi::init();
        return true;
    }

    bool isUp() const override {
        // wifi_manager_get_status() is not free; cache the answer briefly.
        const uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        if (last_check_ms_ != 0 && (now - last_check_ms_) < kCacheMs) {
            return last_state_;
        }
        last_check_ms_ = now;

        wifi_status_t status{};
        last_state_ = wifi::getStatus(status) &&
                      status.ip[0] != '\0' &&
                      std::strcmp(status.ip, "0.0.0.0") != 0;
        return last_state_;
    }

private:
    static constexpr uint32_t kCacheMs = 500;
    mutable uint32_t last_check_ms_ = 0;
    mutable bool     last_state_ = false;
};

} // namespace adapters
