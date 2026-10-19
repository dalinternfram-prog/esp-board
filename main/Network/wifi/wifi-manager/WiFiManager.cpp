







#include "WiFiManager.hpp"











 


/**
 * @brief Callback for WiFi connected event
 */
 void on_wifi_connected(const char *event, const void *data, size_t len, void *ctx)
{
    const wifi_connected_t *info = (const wifi_connected_t *)data;
    ESP_LOGI(TAG, "WiFi connected to %s (RSSI: %d dBm, Channel: %d)",
             info->ssid, info->rssi, info->channel);
}

/**
 * @brief Callback for WiFi disconnected event
 */
 void on_wifi_disconnected(const char *event, const void *data, size_t len, void *ctx)
{
    const wifi_disconnected_t *info = (const wifi_disconnected_t *)data;
    ESP_LOGW(TAG, "WiFi disconnected from %s (reason: %d)", info->ssid, info->reason);
}

/**
 * @brief Callback for got IP event
 */
 void on_wifi_got_ip(const char *event, const void *data, size_t len, void *ctx)
{
    ESP_LOGI(TAG, "Got IP address");

    // Get full status
    wifi_status_t status;
    if (wifi_manager_get_status(&status) == ESP_OK) {
        ESP_LOGI(TAG, "IP: %s", status.ip);
        ESP_LOGI(TAG, "Gateway: %s", status.gateway);
        ESP_LOGI(TAG, "Netmask: %s", status.netmask);
        ESP_LOGI(TAG, "DNS: %s", status.dns);
        ESP_LOGI(TAG, "MAC: %s", status.mac);
        ESP_LOGI(TAG, "Signal quality: %d%%", status.quality);
    }
}

/**
 * @brief Callback for variable changed event
 */
 void on_var_changed(const char *event, const void *data, size_t len, void *ctx)
{
    const wifi_var_t *var = (const wifi_var_t *)data;
    ESP_LOGI(TAG, "Variable changed: %s = %s", var->key, var->value);
}
