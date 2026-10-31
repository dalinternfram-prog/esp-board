
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
#include "file_manager.hpp"
#include "firmware_switch.hpp"
#include "wifi/shared.hpp"
#include "Net manager.hpp" 
#include <stdio.h>
#include <sys/statvfs.h> 
#include " audio_service.hpp"

// [CHANGE]: Included FreeRTOS headers explicitly for xTaskCreatePinnedToCore
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "protocol/server/audio/turn_on_audio.hpp"
/*
add comment to where you change and preserve every comment for future review
*/
constexpr uint32_t conv_id = 2222222;
const char* host ="172.30.165.156";
 const char* port="8080";
 const char* ssid="Dalintern device";
  const char* password= "cykcykcyk";  
   const char* TAG = "app_run_loop";
   using namespace Protocol;
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
AudioService::start();



    //TODO:* this code is used to send handshake with server;we need to turn it on after;
 //   Network::send_handshakeToServer();

    ESP_LOGI("APP_LOOP", "All systems initialized successfully. Entering main loop.");


    while (true) {

//TODO:we need to pull server information;

 std::vector<uint8_t>pulled_result =  Network::poll_and_receive();
if(pulled_result.size()>0){
    
    
    
if( RecordControl::Client::is_record_control(pulled_result)){
 auto result =   RecordControl::Client:: decode(pulled_result);
if(result.has_value()){
    if(result.value().command == RecordControl::RecordCommand::START){
        //TODO turn on the audio;
        if(! AudioService::is_running()){
            AudioService::start();
        }
    }
    else{
           if( AudioService::is_running()){
            AudioService::stop();
        }
    }
}
}
 
  



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