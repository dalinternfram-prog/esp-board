// #include "wifi_manager.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
// #include <string.h>

// static const char *TAG = "WIFI_MANAGER";

// #define WIFI_CONNECTED_BIT BIT0
// #define WIFI_FAIL_BIT      BIT1

// static EventGroupHandle_t s_wifi_event_group;
// static int s_retry_num = 0;
// static bool s_is_connected = false;

// static void event_handler(void* arg, esp_event_base_t event_base,
//                             int32_t event_id, void* event_data) {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         esp_wifi_connect();
//     } 
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*) event_data;
//     ESP_LOGW(TAG, "Disconnected reason: %d", disconn->reason);
//         s_is_connected = false;
//         if (s_retry_num < 10) {
//             esp_wifi_connect();
//             s_retry_num++;
//             ESP_LOGW(TAG, "Retrying Wi-Fi connection (%d/10)...", s_retry_num);
//         } else {
//             xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//         }
//     } 
//     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI(TAG, "Got IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
//         s_retry_num = 0;
//         s_is_connected = true;
//         xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//     }
// }

// esp_err_t wifi_manager_init_sta(const char* ssid, const char* password) {
//     s_wifi_event_group = xEventGroupCreate();

//     // 1. Initialize NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
 
//     // 2. Init Network Stack
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_sta();

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     // 3. Register Event Handlers
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

//     // 4. Configure & Start
//     wifi_config_t wifi_config = {0};
//     strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
//     strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     // Disable Wi-Fi power save mode for low-latency audio streaming
//     esp_wifi_set_ps(WIFI_PS_NONE);

//     // 5. Block until connected or failed
//     EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//             WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//             pdFALSE, pdFALSE, portMAX_DELAY);

//     if (bits & WIFI_CONNECTED_BIT) {
//         ESP_LOGI(TAG, "Connected to SSID: %s", ssid);
//         return ESP_OK;
//     } else {
//         ESP_LOGE(TAG, "Failed to connect to SSID: %s", ssid);
//         return ESP_FAIL;
//     }
// }

// bool wifi_manager_is_connected(void) {
//     return s_is_connected;
// }