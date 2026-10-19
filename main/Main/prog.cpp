  
#include <cstdint>
#include<memory>
#include "Network/Net manager.hpp"


#include <iostream>

#include "audio_capture_driver.hpp"

#include "./audio-collect.hpp"
#include "wifi_init.hpp"
#include "nvs_manager.hpp"
#include <memory>
#include <string>
#include "file_manager.hpp"
#include "firmware_switch.hpp"
#include "wifi_client.hpp"
#include "wifi_provision.hpp"
#include "try-wifi.hpp"
 
//**
// 
// idf.py monitor
// idf.py fullclean
// idf.py erase-flash flash




//  */
/*
add comment to where you change and preserve every comment for future review
*/
constexpr uint32_t conv_id = 2222222;
const char* host ="10.165.11.156";
 const char* port="8080";
 const char* ssid="Dalintern device";
  const char* password= "cykcykcyk"; 
static const char* TAG = "AppLoop";

 

void app_run_loop( 
     std::shared_ptr<NvsManager> nvsManager,
     std::shared_ptr<FileManager> fileManager,
     std::shared_ptr<FirmwareSwitch> firmwareSwitch
){ 
    //* first: we check wifi state;
    // * we try to connect to our wifi;
   //* scan nvs to find the candicate;


     std::shared_ptr<WifiClient> WifiClient_class=std::make_shared<WifiClient>();

     std::shared_ptr<WifiProvision> WifiProvision_class=std::make_shared<WifiProvision>();


     // Package parameters for the background task
    auto* params = new wifi::try_connect::WifiMonitorParams{WifiClient_class, nvsManager, WifiProvision_class};

    // Spawn the background watchdog task and forget about it
    xTaskCreate(
        wifi::try_connect::wifi_background_monitor_task, // Task function pointer
        "wifi_monitor_task",          // Task name
        4096,                         // Stack size
        params,                       // Parameters pointer
        2,                            // Task priority
        NULL
    );

    // Main Application Runtime Loop (Starts instantly and just checks the bool when needed)
    ESP_LOGI(TAG, "Entering main application loop...");




while(1){

            vTaskDelay(pdMS_TO_TICKS(20020));
}

// std::shared_ptr<Network::manager> s_NetworkManager = std::make_shared<Network::manager>(conv_id);
 
//  bool success = s_NetworkManager->initWifi(ssid,password);

// bool success2=  s_NetworkManager->try_connectWithServer(   host,   port);

// std::shared_ptr<Hardware::AudioCaptureDriver> s_AudioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>( );


  

//     while (true) {
//         // 2. Read new PCM frame from hardware
//         esp_err_t ret = s_AudioCaptureDriver->readSamples();
//         if (ret == ESP_OK) {
//             // 3. Get modern C++ std::span pointing to the raw PSRAM data
//             std::span<const int16_t> audio_span = s_AudioCaptureDriver->getFrameBuffer();

//             //ESP_LOGI("++++++++++++","success esp_board_init" );
//             // Print first sample value to verify real PCM data is being captured
//     // if (!audio_span.empty()) {
//     //     ESP_LOGI("++++++++++++", "Captured %zu samples. First sample: %d", 
//     //              audio_span.size(), audio_span[0]);
//     // }

// int16_t max_sample = 0;
//     for (int16_t sample : audio_span) {
//         if (std::abs(sample) > max_sample) {
//             max_sample = std::abs(sample);
//         }
//     }
//     ESP_LOGI("MicTest", "Frame Peak: %d", max_sample);

//     // if (!audio_span.empty()) {
//     //     // Print sample value to observe peak changes
//     //     ESP_LOGI("MicTest", "Sample[0]: %d", audio_span[0]);
//     // }
// // Size in elements (returns 1024)
//     std::size_t element_count = audio_span.size(); 

//     // Size in raw bytes for network transmission (returns 2048)
//     std::size_t byte_count = audio_span.size_bytes(); 

//     ESP_LOGI("BufferCheck", "Elements: %zu | Total Bytes: %zu", element_count, byte_count);
//         } 
        
        
//         else {
//             ESP_LOGW("++++++++++++", "Failed to read audio samples, retrying...");
//         }

//         vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
//     }








} 