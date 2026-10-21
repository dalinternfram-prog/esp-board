  
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h" 
#include "bsp_board.h"
#include "tca9555_driver.h"
#include "mic_speech.h"
#include "audio_driver.h"
#include "rgb_led_driver.h"
#include "button_driver.h"  
#include "macroPhone/arguments.hpp" 
#include "nvs_manager.hpp"
#include <memory>
#include "file_manager.hpp"
#include "firmware_switch.hpp"
#include "wifi/shared.hpp"
#include "Net manager.hpp"
// CHANGED: Added const to fix "ISO C++ forbids converting a string constant to 'char*'" warning
static const char *TAG = "app main";

#define MAX_MP3_FILE    50
static char SD_Name[MAX_MP3_FILE][100]; 
static uint16_t Search_mp3_file_count = 0;
static char music_buf[120] = {0};
static int16_t now_play_id = 0;

void app_run_loop( 
     std::shared_ptr<NvsManager> nvsManager, 
     std::shared_ptr<FirmwareSwitch> firmwareSwitch
);
// 按键回调函数
static void on_key_event(key_id_t id, key_event_t event, void *user_data) 
{
    int vol;
    switch (id) 
    {
        case KEY_ID_9:
            if (event == KEY_EVENT_SHORT_PRESS) 
            {
                vol = get_audio_volume();
                vol+=10;
                if(vol>Volume_MAX)
                    vol = Volume_MAX;
                Volume_Adjustment(vol);
                ESP_LOGI(TAG, "vol=%d",vol);
            } 
            else 
            {
                Audio_Stop_Play();
                if(Search_mp3_file_count==0)
                    return;
                now_play_id--;
                if(now_play_id<0)
                {
                    now_play_id = Search_mp3_file_count-1;
                }
                memset(music_buf,0,sizeof(music_buf));
                sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                Audio_Play_Music(music_buf);
                ESP_LOGI(TAG, "上一首");
            }
            break;
        case KEY_ID_10:
            if (event == KEY_EVENT_SHORT_PRESS) 
            {
                esp_asp_state_t state =  Audio_Get_Current_State();
                switch (state)
                {
                    case ESP_ASP_STATE_NONE:
                        if(Search_mp3_file_count==0)
                            return;
                        memset(music_buf,0,sizeof(music_buf));
                        sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                        Audio_Play_Music(music_buf);
                        set_rgb_mode(RGB_MODE_PLAYING);
                        ESP_LOGI(TAG, "播放:%s",music_buf);
                        break;
                    case ESP_ASP_STATE_RUNNING:
                        ESP_LOGI(TAG, "暂停播放");
                        Audio_Pause_Play();
                        set_rgb_mode(RGB_MODE_IDLE);
                        break;
                    case ESP_ASP_STATE_PAUSED:
                        ESP_LOGI(TAG, "恢复播放");
                        Audio_Resume_Play();
                        set_rgb_mode(RGB_MODE_PLAYING);
                        break;
                    case ESP_ASP_STATE_STOPPED:
                        memset(music_buf,0,sizeof(music_buf));
                        sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                        Audio_Play_Music(music_buf);
                        ESP_LOGI(TAG, "播放:%s",music_buf);
                        set_rgb_mode(RGB_MODE_PLAYING);
                        break;
                    case ESP_ASP_STATE_FINISHED:
                        ESP_LOGI(TAG, "播放完毕");
                        set_rgb_mode(RGB_MODE_IDLE);
                        break;
                    default:
                        break;
                }
                
            } 
            else 
            {
                Audio_Stop_Play();
                ESP_LOGI(TAG, "停止播放");
            }
            break;
        case KEY_ID_11:
            if (event == KEY_EVENT_SHORT_PRESS) 
            {
                vol = get_audio_volume();
                vol-=10;
                if(vol<0)
                    vol = 0;
                Volume_Adjustment(vol);
                ESP_LOGI(TAG, "vol=%d",vol);
            } 
            else 
            {
                Audio_Stop_Play();
                if(Search_mp3_file_count==0)
                    return;
                now_play_id++;
                if(now_play_id==Search_mp3_file_count)
                {
                    now_play_id = 0;
                }
                memset(music_buf,0,sizeof(music_buf));
                sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                Audio_Play_Music(music_buf);
                ESP_LOGI(TAG, "上一首");
            }
            break;
    }
}

//语音唤醒和命令词回调函数
static void Speech_event_callback(esp_sr_rec_event_t event,esp_sr_evt_data_t evt_data, void *user_data)
{
    static bool play_flag = 0;
    switch (event)
    {
        // CHANGED: Enclosed case block in {} scope to fix cross-initialization compile errors in C++
        case ESP_SR_EVT_AWAKEN: {
            ESP_LOGI(TAG, "ESP_SR_EVT_AWAKEN = %d",evt_data.awaken_channel);
            esp_asp_state_t state =  Audio_Get_Current_State();
            if(state==ESP_ASP_STATE_RUNNING)
            {
                ESP_LOGI(TAG, "暂停播放");
                Audio_Pause_Play();
                play_flag = 1;
            }
            set_rgb_mode(RGB_MODE_REC_COMMAND);
            break;
        }
        // CHANGED: Enclosed case block in {} scope
        case ESP_SR_EVT_CMD: {
            //ESP_LOGI(TAG, "ESP_SR_EVT_CMD = %d",evt_data.sr_cmd);
            switch (evt_data.sr_cmd)
            {
                case 0:
                    ESP_LOGI(TAG, "灯光变成红色");
                    set_rgb_color(RGB_COLOR_RED);
                    break;
                case 1:
                    ESP_LOGI(TAG, "灯光变成蓝色");
                    set_rgb_color(RGB_COLOR_BLUE);
                    break;
                case 2:
                    ESP_LOGI(TAG, "灯光变成绿色");
                    set_rgb_color(RGB_COLOR_GREEN);
                    break;
                case 3:
                    ESP_LOGI(TAG, "灯光变成白色");
                    set_rgb_color(RGB_COLOR_WHITE);
                    break;
                default:
                    break;
            }
            break;
        }
        // CHANGED: Enclosed case block in {} scope
        case ESP_SR_EVT_CMD_TIMEOUT: {
            ESP_LOGI(TAG, "ESP_SR_EVT_CMD_TIMEOUT");

            if(play_flag == 1)
            {
                play_flag = 0;
                ESP_LOGI(TAG, "恢复播放");
                Audio_Resume_Play();
                set_rgb_mode(RGB_MODE_PLAYING);
            }
            else
            {
                set_rgb_mode(RGB_MODE_IDLE);
            }
            break;
        }
        default:
            break;
    }
}

//扫描TF卡MP3文件
static void Search_mp3_Music(void)     
{        
    // CHANGED: Cast string literal to char* to fix C++ string conversion warning
    Search_mp3_file_count = Folder_retrieval(const_cast<char*>("/sdcard"), const_cast<char*>(".mp3"), SD_Name, MAX_MP3_FILE);
    printf("file_count=%d\r\n",Search_mp3_file_count);
    if(Search_mp3_file_count) 
    {  
        for (int i = 0; i < Search_mp3_file_count; i++) 
        {
            ESP_LOGI("SAFASF","%s",SD_Name[i]);
        }                
        
    }                                                               
}

extern "C" void app_main()
{ 

            ESP_LOGI("++++++++++++","Hello,can you hear me???" );
    ESP_ERROR_CHECK(esp_board_init(macroPhone::argument::sample_rate, macroPhone::argument::channel_count, macroPhone::argument::bits_per_chan));

            ESP_LOGI("++++++++++++","success esp_board_init" );
   tca9555_driver_init();

            ESP_LOGI("++++++++++++","success tca9555_driver_init" );
   // CHANGED: Cast string literal to char* to fix C++ string conversion warning
   esp_sdcard_init(const_cast<char*>("/sdcard"), 10);
    
            ESP_LOGI("++++++++++++","success esp_sdcard_init" );
   //Speech_Init();

            ESP_LOGI("++++++++++++","success Speech_Init" );
   Speech_register_callback(Speech_event_callback);

            ESP_LOGI("++++++++++++","success Speech_register_callback" );
    Audio_Play_Init();

            ESP_LOGI("++++++++++++","success Audio_Play_Init" );
     RGB_Example();

            ESP_LOGI("++++++++++++","success RGB_Example" );
     key_module_init(NULL);

            ESP_LOGI("++++++++++++","success key_module_init" );
     key_register_callback(on_key_event);
    
            ESP_LOGI("++++++++++++","success key_register_callback" );
     Search_mp3_Music();

            ESP_LOGI("++++++++++++","success Search_mp3_Music" );
 

std::shared_ptr<NvsManager> NvsManager_class=std::make_shared<NvsManager>();
 

std::shared_ptr<FirmwareSwitch> FirmwareSwitch_class=std::make_shared<FirmwareSwitch>();


NvsManager_class->init(); 
FirmwareSwitch_class->init();

FileManager::init();


wifi::init();

Network::init(222);
 app_run_loop( NvsManager_class, FirmwareSwitch_class);

}


// /**
//  * @file main.c
//  * @brief ESP WiFi Manager - Web UI Example
//  *
//  * This example demonstrates:
//  * - WiFi Manager with embedded Web UI
//  * - Modern responsive interface accessible at device IP
//  * - Captive portal for initial setup
//  * - mDNS for easy access (esp32-xxx.local)
//  */

// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_wifi_manager.h"
// #include "esp_bus.h"

// static const char *TAG = "wifi_webui_example";

// /**
//  * @brief Callback for WiFi events
//  */
//  extern "C" {
//   void on_wifi_event(const char *event, const void *data, size_t len, void *ctx)
// {
//     if (strcmp(event, WIFI_EVT(WIFI_MGR_EVT_CONNECTED)) == 0) {
//         const wifi_connected_t *info = (const wifi_connected_t *)data;
//         ESP_LOGI(TAG, "+++++++++++++++++++++++++++Connected to %s", info->ssid);
//     } else if (strcmp(event, WIFI_EVT(WIFI_MGR_EVT_DISCONNECTED)) == 0) {
//         const wifi_disconnected_t *info = (const wifi_disconnected_t *)data;
//         ESP_LOGW(TAG, "+++++++++++++++++++++++++++Disconnected from %s (reason: %d)", info->ssid, info->reason);
//     } else if (strcmp(event, WIFI_EVT(WIFI_MGR_EVT_GOT_IP)) == 0) {
//         wifi_status_t status;
//         if (wifi_manager_get_status(&status) == ESP_OK) {
//             ESP_LOGI(TAG, "+++++++++++++++++++++++++++Got IP: %s", status.ip);
//             ESP_LOGI(TAG, "+++++++++++++++++++++++++++Web UI: http://%s/", status.ip);
//             if (status.hostname[0]) {
//                 ESP_LOGI(TAG, "+++++++++++++++++++++++++++mDNS: http://%s.local/", status.hostname);
//             }
//         }
//         }
// }
//  }
// extern "C" void app_main(void)
// {
//     // Initialize NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     ESP_LOGI(TAG, "==============================================");
//     ESP_LOGI(TAG, "  ESP WiFi Manager - Web UI Example");
//     ESP_LOGI(TAG, "==============================================");

//     // Initialize esp_bus
//     ESP_ERROR_CHECK(esp_bus_init());

//     // Subscribe to WiFi events
//     esp_bus_sub("wifi_mgr_evt_connected", on_wifi_event, NULL);
//     esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_DISCONNECTED), on_wifi_event, NULL);
//     esp_bus_sub(WIFI_EVT(WIFI_MGR_EVT_GOT_IP), on_wifi_event, NULL);

//     // Initialize WiFi Manager with Web UI enabled
//     wifi_manager_config_t config = {
//         .max_retry_per_network = 3,
//         .retry_interval_ms = 5000,
//         .auto_reconnect = true,

//         .default_ap = {
//             .ssid = "ESP32-Setup-{id}",
//             .password = "",
//             .channel = 0,
//             .max_connections = 4,
//             .ip = "192.168.4.1",
//             .netmask = "255.255.255.0",
//             .gateway = "192.168.4.1",
//             .dhcp_start = "192.168.4.2",
//             .dhcp_end = "192.168.4.20",
//         },
//         .enable_captive_portal = true,
//         .stop_ap_on_connect = true,

//         .http = {
//             .enable = true,
//             .api_base_path = "/api/wifi",
//         },

//         // mDNS for easy access
//         .mdns = {
//             .enable = true,
//             .hostname = "esp32-{id}",
//             .instance_name = "ESP32 WiFi Manager",
//         },

//         // Web UI is auto-enabled via CONFIG_WIFI_MGR_ENABLE_WEBUI
//     };

//     ret = wifi_manager_init(&config);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to init WiFi Manager: %s", esp_err_to_name(ret));
//         return;
//     }

//     ESP_LOGI(TAG, "");
//     ESP_LOGI(TAG, "WiFi Manager initialized with Web UI");
//     ESP_LOGI(TAG, "");

//     // Wait for connection
//     ret = wifi_manager_wait_connected(30000);

//     if (ret == ESP_OK) {
//         wifi_status_t status;
//         wifi_manager_get_status(&status);
//         ESP_LOGI(TAG, "Connected! Access Web UI at:");
//         ESP_LOGI(TAG, "  http://%s/", status.ip);
//         if (status.hostname[0]) {
//             ESP_LOGI(TAG, "  http://%s.local/", status.hostname);
//         }
//     } else {
//         ESP_LOGW(TAG, "No saved networks. Starting AP mode...");
//         ESP_LOGI(TAG, "");
//         ESP_LOGI(TAG, "Connect to WiFi: ESP32-Setup-XXXXXX");
//         ESP_LOGI(TAG, "Open http://192.168.4.1/ to configure");
//         ESP_LOGI(TAG, "");
//     }

//     // Main loop
//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(10000));
//     }
// }
