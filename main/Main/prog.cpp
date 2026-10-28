  
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

/*
TODO 10/27 18:25,error log:

E (11274) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (11274) task_wdt:  - IDLE1 (CPU 1)
E (11274) task_wdt: Tasks currently running:
E (11274) task_wdt: CPU 0: IDLE0
E (11274) task_wdt: CPU 1: app_run_loop_ta
E (11274) task_wdt: Print CPU 1 backtrace


Backtrace: 0x40378E52:0x3FCA1BC0 0x40378725:0x3FCA1BE0 0x4218E091:0x3FCC33D0 0x420B074E:0x3FCC3540 0x420AEFC0:0x3FCC38C0 0x4207F7B7:0x3FCC6330 0x4205B70D:0x3FCC6680 0x4205C4E5:0x3FCC6DC0 0x42058367:0x3FCC6E00 0x4204361D:0x3FCC6E40 0x42014BC3:0x3FCC6E60 0x42014C30:0x3FCC6EB0 0x42010F5D:0x3FCC6EE0 0x40384C21:0x3FCC6FE0
--- 0x40378e52: esp_crosscore_isr at /home/emily/.espressif/v6.1/esp-idf/components/esp_system/crosscore_int.c:74
--- 0x40378725: _xt_lowint1 at /home/emily/.espressif/v6.1/esp-idf/components/xtensa/xtensa_vectors.S:1264
--- 0x4218e091: silk_warped_autocorrelation_FIX_c at ??:?
--- 0x420b074e: silk_noise_shape_analysis_FIX at ??:?
--- 0x420aefc0: silk_encode_frame_FIX at ??:?
--- 0x4207f7b7: silk_Encode at ??:?
--- 0x4205b70d: opus_encode_native at ??:?
--- 0x4205c4e5: opus_encode at ??:?
--- 0x42058367: esp_opus_enc_process at ??:?
--- 0x4204361d: esp_audio_enc_process at ??:?
--- 0x42014bc3: SimpleOpusEncoder::encode(std::vector<short, std::allocator<short> > const&, unsigned char*, unsigned long) at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/macroPhone/Opus/Opus.cpp:793
--- 0x42014c30: SimpleOpusEncoder::encode(std::vector<short, std::allocator<short> > const&, unsigned long) at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/macroPhone/Opus/Opus.cpp:817
--- 0x42010f5d: app_run_loop(void*) at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/Main/prog.cpp:178
--- 0x40384c21: vPortTaskWrapper at /home/emily/.espressif/v6.1/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:147


*/


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
        //  auto compressed_pcm = s_SimpleOpusEncoder->encode(raw_pcm);

  
// Size in elements (returns 1024)
    std::size_t element_count = raw_pcm.size(); 

    // Size in raw bytes for network transmission (returns 2048)
    std::size_t byte_count = raw_pcm.size()*sizeof(int16_t); 
 
// Correctly copy the entire byte representation (elements * sizeof(int16_t))
const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(raw_pcm.data());

    // Split into two UDP payloads
    const std::size_t packet_size = byte_count / 2;


        std::vector<unsigned char> packet1(
        byte_ptr,
        byte_ptr + packet_size
    );

    std::vector<unsigned char> packet2(
        byte_ptr + packet_size,
        byte_ptr + byte_count
    );

        // Send both packets
    bool success1 = Network::send(packet1) != -1;
    bool success2 = Network::send(packet2) != -1;


//std::vector<unsigned char> raw_pcm_(byte_ptr, byte_ptr + byte_count);
//TODO: the result,we need to send to server;
//bool success = Network::send(raw_pcm_)!=-1;



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




 