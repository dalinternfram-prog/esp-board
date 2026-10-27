  
#include <cstdint>
#include<memory>
#include "Network/Net manager.hpp"

#include "esp_timer.h"
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
#include <stdio.h>
#include <sys/statvfs.h>
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


void kcp_update_task(void *pvParameters) {
   
    while (true) {
        // Get current time in milliseconds
        uint32_t current_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    
        Network::update(current_ms);
        // Run every 10 milliseconds
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void send_handshake_task(void *pvParameters) {
   
    while (true) {
    if(Network::completeHandshake){
  //TODO delete current task;
  vTaskDelete(NULL);
    } 
  

  
    if(!wifi::isConnected()){

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    else{
         Network::send_handshakeToServer();
           vTaskDelete(NULL);

    }
    
        vTaskDelay(pdMS_TO_TICKS(10));
       } 
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

// Example of how to launch the task:
xTaskCreate(
    kcp_update_task,      // Task function
    "kcp_update_task",    // Task name (for debugging)
    4096,                 // Stack size in bytes (4KB is usually plenty)
    NULL,                 // Parameters to pass (e.g., your KCP pointer if needed)
    5,                    // Task priority (keep it moderate, e.g., 5)
    NULL                  // Task handle (optional)
);
xTaskCreate(
    send_handshake_task,      // Task function
    "send_handshake_task",    // Task name (for debugging)
    4096,                 // Stack size in bytes (4KB is usually plenty)
    NULL,                 // Parameters to pass (e.g., your KCP pointer if needed)
    5,                    // Task priority (keep it moderate, e.g., 5)
    NULL                  // Task handle (optional)
);



    //TODO:* this code is used to send handshake with server;we need to turn it on after;
 //   Network::send_handshakeToServer();

    ESP_LOGI("APP_LOOP", "All systems initialized successfully. Entering main loop.");



 
  

std::shared_ptr<Hardware::AudioCaptureDriver> s_AudioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>( );

std::shared_ptr< SimpleOpusEncoder> s_SimpleOpusEncoder = std::make_shared< SimpleOpusEncoder>(
     SimpleOpusEncoder::Config{macroPhone::argument::sample_rate,macroPhone::argument::channel_count,macroPhone::argument::target_frame_duration_ms} );
auto ret=s_SimpleOpusEncoder->init();
if(ret!=ESP_OK) ESP_LOGE(TAG, "s_SimpleOpusEncoder->init(); error");

 

    while (true) {


//TODO:we need to pull server information;

 std::vector<uint8_t>pulled_result =  Network::poll_and_receive();
if(pulled_result.size()>0){
    //TODO: handle the result;

    ESP_LOGI("Server", "Get server information");
}



        // 2. Read new PCM frame from hardware
         ret = s_AudioCaptureDriver->readSamples();
        if (ret == ESP_OK) {
            // 3. Get modern C++ std::span pointing to the raw PSRAM data
           auto raw_pcm = s_AudioCaptureDriver->getFrameBuffer();
            auto compressed_pcm = s_SimpleOpusEncoder->encode(raw_pcm);

         // ESP_LOGI("++++++++++++","auto compressed_pcm = s_SimpleLC3Encoder->encode(raw_pcm); success  size = %u",compressed_pcm.size() );
            // Print first sample value to verify real PCM data is being captured
    // if (!audio_span.empty()) {
    //     ESP_LOGI("++++++++++++", "Captured %zu samples. First sample: %d", 
    //              audio_span.size(), audio_span[0]);
    // }
if (!raw_pcm.empty()) {
    int16_t min_val = 32767;
    int16_t max_val = -32768;
    int64_t sum_magnitude = 0;

    for (int16_t sample : raw_pcm) {
        if (sample < min_val) min_val = sample;
        if (sample > max_val) max_val = sample;
        sum_magnitude += std::abs(static_cast<int32_t>(sample));
    }

    int32_t avg_magnitude = sum_magnitude / raw_pcm.size();

    // ESP_LOGI("CLIENT_AUDIO", "Size: %zu samples | Min: %d | Max: %d | Avg Mag: %d",
    //          raw_pcm.size(), min_val, max_val, (int)avg_magnitude);
}
int16_t max_sample = 0;
    for (int16_t sample : raw_pcm) {
        if (std::abs(sample) > max_sample) {
            max_sample = std::abs(sample);
        }
    }
    //ESP_LOGI("MicTest", "Frame Peak: %d", max_sample);

    // if (!audio_span.empty()) {
    //     // Print sample value to observe peak changes
    //     ESP_LOGI("MicTest", "Sample[0]: %d", audio_span[0]);
    // }
// Size in elements (returns 1024)
    std::size_t element_count = raw_pcm.size(); 

    // Size in raw bytes for network transmission (returns 2048)
    std::size_t byte_count = raw_pcm.size()*sizeof(int16_t); 
 
// Correctly copy the entire byte representation (elements * sizeof(int16_t))
const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(raw_pcm.data());
std::vector<unsigned char> raw_pcm_(byte_ptr, byte_ptr + byte_count);
//TODO: the result,we need to send to server;
bool success = Network::send(raw_pcm_)!=-1;



  ESP_LOGI("BufferCheck", "Elements: %zu | Total Bytes: %zu", element_count, byte_count);
        } 
        
        
        else {
            ESP_LOGW("++++++++++++", "Failed to read audio samples, retrying...");
        }

      //vTaskDelay(pdMS_TO_TICKS(5)); // Yield to keep Task Watchdog happy
    }









    // (Optional cleanup if the loop ever breaks)
    FileManager::deinit();
    vTaskDelete(NULL);
}




 