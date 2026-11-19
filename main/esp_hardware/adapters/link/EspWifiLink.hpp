#pragma once

// Adapter: WiFi connectivity through the existing WiFi manager (ILink port).
// "Up" means associated AND holding a real IP (DHCP completed), so the session
// handshake cannot start before the stack is actually usable.
// Swap this for a Cellular4GLink / EthernetLink later without touching the core.

#include "interface.hpp"

#include <atomic>
#include <cstring>

#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "WiFiManager.hpp"
#include "wifi/shared.hpp"

namespace adapters {

class EspWifiLink : public core::ILink {
public:
    bool start() override {
        wifi::init();   // creates esp_netif + the default event loop

        // Only now is esp_netif_sntp_init() valid: SNTP requires esp_netif and
        // the default event loop to already exist (else it silently fails and
        // nowUnixMs() stays at 1970).
        static bool sntp_started = false;
        if (!sntp_started) {
            esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
            sntp_started = (esp_netif_sntp_init(&config) == ESP_OK);
        }
        return true;
    }

    bool isUp() const override {
        // wifi_manager_get_status() is not free; cache the answer briefly.
        const uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        const uint32_t last = last_check_ms_.load();
        if (last != 0 && (now - last) < kCacheMs) {
            return last_state_.load();
        }

        wifi_status_t status{};
        const bool up = wifi::isConnected() &&
                        wifi::getStatus(status) &&
                        status.ip[0] != '\0' &&
                        std::strcmp(status.ip, "0.0.0.0") != 0;
        last_state_.store(up);
        last_check_ms_.store(now);
        return up;
    }

private:
    static constexpr uint32_t kCacheMs = 500;
    mutable std::atomic<uint32_t> last_check_ms_{0};
    mutable std::atomic<bool>     last_state_{false};
};

} // namespace adapters
