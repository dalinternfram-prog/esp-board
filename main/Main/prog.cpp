  
#include <cstdint>
#include<memory>
#include "Network/Net manager.hpp"


#include <iostream>

#include "audio_capture_driver.hpp"

#include "./audio-collect.hpp" 
#include "macroPhone/arguments.hpp"
#include "nvs_manager.hpp"
#include <memory>
#include <string>
#include "file_manager.hpp"
#include "firmware_switch.hpp" 
#include "WiFiManager.hpp" 
#include "Opus.hpp"
#include "Lc3.hpp"
#include "file_manager.hpp"
#include "firmware_switch.hpp"
#include "wifi/shared.hpp"
#include "Net manager.hpp"
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
   const char* TAG = "app_run_loop";
 

void app_run_loop( 
     std::shared_ptr<NvsManager> nvsManager, 
     std::shared_ptr<FirmwareSwitch> firmwareSwitch
){ 
         esp_err_t ret;
    //* first: we check wifi state;
    // * we try to connect to our wifi;
   //* scan nvs to find the candicate;

   //WiFiManager::getInstance().begin(); 

//      std::shared_ptr<wifi_state> wifi_state_class=std::make_shared<wifi_state>();
//      std::shared_ptr<WifiProvision> WifiProvision_class=std::make_shared<WifiProvision>();
// wifi_state_class->register_handlers();

     // Package parameters for the background task
   // auto* params = new wifi::try_connect::WifiMonitorParams{wifi_state_class, nvsManager, WifiProvision_class};

    // Spawn the background watchdog task and forget about it
//     xTaskCreate(
//         wifi::try_connect::wifi_background_monitor_task, // Task function pointer
//         "wifi_monitor_task",          // Task name
//         4096,                         // Stack size
//         params,                       // Parameters pointer
//         2,                            // Task priority
//         NULL
//     );

//     // Main Application Runtime Loop (Starts instantly and just checks the bool when needed)
//     ESP_LOGI(TAG, "Entering main application loop...");




// while(1){

//             vTaskDelay(pdMS_TO_TICKS(20020));
// }

// std::shared_ptr<Network::manager> s_NetworkManager = std::make_shared<Network::manager>(conv_id);
 
//  bool success = s_NetworkManager->initWifi(ssid,password);

// bool success2=  s_NetworkManager->try_connectWithServer(   host,   port);

// std::shared_ptr<Hardware::AudioCaptureDriver> s_AudioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>( );

// std::shared_ptr< SimpleLC3Encoder> s_SimpleLC3Encoder = std::make_shared< SimpleLC3Encoder>(
//      SimpleLC3Encoder::Config{macroPhone::argument::sample_rate,macroPhone::argument::channel_count,macroPhone::argument::target_frame_duration_ms} );
// ret=s_SimpleLC3Encoder->init();
// if(ret!=ESP_OK) ESP_LOGE(TAG, "s_SimpleLC3Encoder->init(); error");

 

//     while (true) {
//         // 2. Read new PCM frame from hardware
//          ret = s_AudioCaptureDriver->readSamples();
//         if (ret == ESP_OK) {
//             // 3. Get modern C++ std::span pointing to the raw PSRAM data
//            auto raw_pcm = s_AudioCaptureDriver->getFrameBuffer();
//             auto compressed_pcm = s_SimpleLC3Encoder->encode(raw_pcm);

//          // ESP_LOGI("++++++++++++","auto compressed_pcm = s_SimpleLC3Encoder->encode(raw_pcm); success  size = %u",compressed_pcm.size() );
//             // Print first sample value to verify real PCM data is being captured
//     // if (!audio_span.empty()) {
//     //     ESP_LOGI("++++++++++++", "Captured %zu samples. First sample: %d", 
//     //              audio_span.size(), audio_span[0]);
//     // }

// int16_t max_sample = 0;
//     for (int16_t sample : raw_pcm) {
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
//     std::size_t element_count = raw_pcm.size(); 

//     // Size in raw bytes for network transmission (returns 2048)
//     std::size_t byte_count = raw_pcm.size()*sizeof(int16_t); 

//    ESP_LOGI("BufferCheck", "Elements: %zu | Total Bytes: %zu", element_count, byte_count);
//         } 
        
        
//         else {
//             ESP_LOGW("++++++++++++", "Failed to read audio samples, retrying...");
//         }

//         vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
//     }








} 
// This function acts as the FreeRTOS task routine
void app_run_loop(void* pvParameters) {
    ESP_LOGI("APP_LOOP", "Starting application run loop task...");

    // 1. Instantiate core components
    auto nvsManager = std::make_shared<NvsManager>();
    auto firmwareSwitch = std::make_shared<FirmwareSwitch>();

    // 2. Initialize drivers and services
    nvsManager->init();
    firmwareSwitch->init();
    FileManager::init();
    wifi::init();
    Network::init(222);

    ESP_LOGI("APP_LOOP", "All systems initialized successfully. Entering main loop.");

    // 3. Main Application Run Loop
    while (true) {
        // TODO: Perform periodic application tasks here 
        // (e.g., polling sensors, checking network status, handling state transitions)

        // Yield control to FreeRTOS (e.g., delay for 1 second)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // (Optional cleanup if the loop ever breaks)
    FileManager::deinit();
    vTaskDelete(NULL);
}




 