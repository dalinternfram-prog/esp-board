  
// #include <cstdint>
// #include<memory>
// #include "Network/Net manager.hpp"

// #include "esp_timer.h"
// #include <iostream>

// #include "audio_capture_driver.hpp"
 
// #include "macroPhone/arguments.hpp"
// #include "nvs_manager.hpp"
// #include <memory>
// #include <string>
// #include "file_manager.hpp"
// #include "firmware_switch.hpp" 
// #include "WiFiManager.hpp" 
// #include "Opus.hpp"
// #include "Lc3.hpp"
// #include "file_manager.hpp"
// #include "firmware_switch.hpp"
// #include "wifi/shared.hpp"
// #include "Net manager.hpp" 
// #include <stdio.h>
// #include <sys/statvfs.h>
// #include "Opus_class.hpp"

// /*
// add comment to where you change and preserve every comment for future review
// */
// constexpr uint32_t conv_id = 2222222;
// const char* host ="172.30.165.156";
//  const char* port="8080";
//  const char* ssid="Dalintern device";
//   const char* password= "cykcykcyk";  
//    const char* TAG = "app_run_loop";
// void check_filesystem_space() {
//     struct statvfs vfs;
    
//     // Replace "/spiffs" or "/littlefs" with your board's mount point
//     if (statvfs("/spiffs", &vfs) == 0) {
//         unsigned long long total = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
//         unsigned long long free = (unsigned long long)vfs.f_bfree * vfs.f_frsize;
//         unsigned long long used = total - free;

//         ESP_LOGE(TAG, "Total space: %llu bytes", total);
//         ESP_LOGE(TAG, "Free space:  %llu bytes", free);
//         ESP_LOGE(TAG, "Used space:  %llu bytes", used);
//     } else {
//         ESP_LOGE(TAG, "Failed to get filesystem statistics");
//     }

//     // Entering main loop log
//     ESP_LOGE(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++All systems initialized successfully. Entering main loop.");
// }




// // This function acts as the FreeRTOS task routine
// void app_run_loop(void* pvParameters) {
//     ESP_LOGI("APP_LOOP", "Starting application run loop task...");
// check_filesystem_space();
//     // 1. Instantiate core components
//     auto nvsManager = std::make_shared<NvsManager>();
//     auto firmwareSwitch = std::make_shared<FirmwareSwitch>();

//     // 2. Initialize drivers and services
//     nvsManager->init();
//     firmwareSwitch->init();
//     FileManager::init();
//     wifi::init();
//     Network::init(host,port);




//     //TODO:* this code is used to send handshake with server;we need to turn it on after;
//  //   Network::send_handshakeToServer();

//     ESP_LOGI("APP_LOOP", "All systems initialized successfully. Entering main loop.");



 
  

// std::shared_ptr<Hardware::AudioCaptureDriver> s_AudioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>( );



// std::shared_ptr<Opus> s_Opus = std::make_shared<Opus>();
// s_Opus->init(macroPhone::argument::sample_rate,macroPhone::argument::channel_count,macroPhone::argument::bits_per_chan,macroPhone::argument::target_frame_duration_ms);






// uint32_t frameCount = 0;

//     while (true) {




 
// //TODO:we need to pull server information;

//  std::vector<uint8_t>pulled_result =  Network::poll_and_receive();
// if(pulled_result.size()>0){
//     //TODO: handle the result;

//     ESP_LOGI("Server", "Get server information");
// }

//         // 2. Read new PCM frame from hardware
//          auto ret = s_AudioCaptureDriver->readSamples();
//         if (ret == ESP_OK) {
//             // 3. Get modern C++ std::span pointing to the raw PSRAM data
//            auto raw_pcm = s_AudioCaptureDriver->getFrameBuffer(); 
//           auto compressed_pcm = s_Opus->encode(raw_pcm.data()); 
//           Network::send(compressed_pcm); 
//         } 
        
//         else {
//             ESP_LOGW("++++++++++++", "Failed to read audio samples, retrying...");
//         }

//       vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
//     }

//     // (Optional cleanup if the loop ever breaks)
//     FileManager::deinit();
//     vTaskDelete(NULL);
// }




 

















#include <cstdint>
#include <memory>
#include "Network/Net manager.hpp"

#include "esp_timer.h"
#include <iostream>

#include "audio_capture_driver.hpp"
 
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
#include <stdio.h>
#include <sys/statvfs.h> 

// [CHANGE]: Included FreeRTOS headers explicitly for xTaskCreatePinnedToCore
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
add comment to where you change and preserve every comment for future review
*/
constexpr uint32_t conv_id = 2222222;
const char* host ="172.30.165.156";
 const char* port="8080";
 const char* ssid="Dalintern device";
  const char* password= "cykcykcyk";  
   const char* TAG = "app_run_loop";
void check_filesystem_space() {
    struct statvfs vfs;
    
    // Replace "/spiffs" or "/littlefs" with your board's mount point
    if (statvfs("/spiffs", &vfs) == 0) {
        unsigned long long total = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
        unsigned long long free = (unsigned long long)vfs.f_bfree * vfs.f_frsize;
        unsigned long long used = total - free;

        ESP_LOGE(TAG, "Total space: %llu bytes", total);
        ESP_LOGE(TAG, "Free space:  %llu bytes", free);
        ESP_LOGE(TAG, "Used space:  %llu bytes", used);
    } else {
        ESP_LOGE(TAG, "Failed to get filesystem statistics");
    }

    // Entering main loop log
    ESP_LOGE(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++All systems initialized successfully. Entering main loop.");
}

// ============================================================================
// [CHANGE]: New Dedicated Audio Processing Task Pinned to Core 1
// ============================================================================
// Separating audio sampling and Opus encoding onto Core 1 prevents audio jitter
// and leaves Core 0 completely free to service the Wi-Fi and TCP/IP stack.
void audio_processing_task(void* pvParameters) {
    ESP_LOGI("AUDIO_TASK", "Audio processing task started on Core %d", xPortGetCoreID());

    // [CHANGE]: Moved hardware driver initialization into this dedicated task
    std::shared_ptr<Hardware::AudioCaptureDriver> s_AudioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>();

    // [CHANGE]: Moved Opus encoder initialization into this dedicated task
    std::shared_ptr<Opus> s_Opus = std::make_shared<Opus>();
    s_Opus->init(
        macroPhone::argument::sample_rate,
        macroPhone::argument::channel_count,
        macroPhone::argument::bits_per_chan,
        macroPhone::argument::target_frame_duration_ms
    );

    uint32_t frameCount = 0;

    while (true) {
        // 2. Read new PCM frame from hardware
        auto ret = s_AudioCaptureDriver->readSamples();
        if (ret == ESP_OK) {
            // 3. Get modern C++ std::span pointing to the raw PSRAM data
            auto raw_pcm = s_AudioCaptureDriver->getFrameBuffer(); 
            auto compressed_pcm = s_Opus->encode(raw_pcm.data()); 

            // [CHANGE]: Network::send is thread-safe (producer-consumer double buffer),
            // so sending from Core 1 to the network thread on Core 0 is safe.
            Network::send(compressed_pcm); 
            frameCount++;
        } 
        else {
            ESP_LOGW("++++++++++++", "Failed to read audio samples, retrying...");
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
    }

    vTaskDelete(NULL);
}


// This function acts as the FreeRTOS task routine
void app_run_loop(void* pvParameters) {
    ESP_LOGI("APP_LOOP", "Starting application run loop task...");
check_filesystem_space();
    // 1. Instantiate core components
    auto nvsManager = std::make_shared<NvsManager>();
    auto firmwareSwitch = std::make_shared<FirmwareSwitch>();

    // 2. Initialize drivers and services
    nvsManager->init();
    firmwareSwitch->init();
    FileManager::init();
    wifi::init();
    Network::init(host,port);




    //TODO:* this code is used to send handshake with server;we need to turn it on after;
 //   Network::send_handshakeToServer();

    ESP_LOGI("APP_LOOP", "All systems initialized successfully. Entering main loop.");

    // =========================================================================
    // [CHANGE]: Launch Audio Capture & Opus Encode Task pinned to CPU Core 1
    // =========================================================================
    // Stack size is 16384 bytes (16KB) to safely accommodate Opus internal frame buffers.
    // Priority is set to 5 (higher priority) to avoid dropping audio I2S DMA buffers.
    xTaskCreatePinnedToCore(
        audio_processing_task,
        "audio_proc_task",
        16384,                  // Stack size in bytes
        nullptr,                // Task parameters
        5,                      // Task Priority (Higher than standard tasks)
        nullptr,                // Task handle
        1                       // Pinned to CPU Core 1
    );

    while (true) {

//TODO:we need to pull server information;

 std::vector<uint8_t>pulled_result =  Network::poll_and_receive();
if(pulled_result.size()>0){
    //TODO: handle the result;

    ESP_LOGI("Server", "Get server information");
}

        // [CHANGE]: Audio capture and encoding routine was moved out of this loop
        // and into `audio_processing_task` on Core 1 to prevent blocking network polling.

      vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
    }

    // (Optional cleanup if the loop ever breaks)
    FileManager::deinit();
    vTaskDelete(NULL);
}