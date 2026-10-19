#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Wi-Fi stack and block/notify until an IP is acquired
esp_err_t wifi_manager_init_sta(const char* ssid, const char* password);

// Check current network status without touching ESP-IDF internals
bool wifi_manager_is_connected(void);

#ifdef __cplusplus
}
#endif