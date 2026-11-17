//TODO though we may switch to SIM,the wifi manager is not as good as expected
//TODO i'm not sure whether i will fix it nor not
//TODO:

//TODO another things,the LED light,we should leave a physical button for voice record,
//TODO but still,i don't quite sure,
//TODO;and we don't need speech driver;




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
#include "turn_on_audio.hpp"
#include "IoT_identity_token.hpp"
/*
add comment to where you change and preserve every comment for future review
*/
constexpr uint32_t conv_id = 2222222;
const char* host ="172.30.165.156";
 const char* port="8080";
 const char* ssid="Dalintern device";
  const char* password= "cykcykcyk";  
   const char* TAG = "app_run_loop";
   std::string DeviceID = "X";
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


// 2. Encode the device query packet using your new protocol
    std::vector<uint8_t> registration_packet = protocol::IoT::client::encodeDeviceQuery(DeviceID);
    // 3. Send the packet to the server over your network connection
    Network::send(registration_packet);

    ESP_LOGI("SERVER", "Sent device registration query for ID: %s", DeviceID.c_str());
   // AudioService::start();



    //TODO:* this code is used to send handshake with server;we need to turn it on after;
 //   Network::send_handshakeToServer();

    ESP_LOGI("APP_LOOP", "All systems initialized successfully. Entering main loop.");


    while (true) {
  Network::send(registration_packet);
//TODO:we need to pull server information;

 std::vector<uint8_t>pulled_result =  Network::poll_and_receive();
if(pulled_result.size()>0){
    

 // Check if the incoming packet matches the RecordControlInfo protocol magic & structure
        if (protocol::IoT::client::isRecordControl(pulled_result)) {
            protocol::IoT::RecordControlInfo info;
            
            if (protocol::IoT::client::decodeRecordControl(pulled_result, info)) {
                
                if (info.command == protocol::IoT::RecordCommand::START) {
                    // Turn on audio recording if it isn't already running
                    if (!AudioService::is_running()) {
                        AudioService::start();
                        ESP_LOGI("Server", "Audio service started via server command (Timestamp: %u)", info.timestampMs);
                    }
                } 
                else if (info.command == protocol::IoT::RecordCommand::STOP) {
                    // Stop audio recording if it is active
                    if (AudioService::is_running()) {
                        AudioService::stop();
                        ESP_LOGI("Server", "Audio service stopped via server command.");
                    }
                }
            } else {
                ESP_LOGW("Server", "Failed to decode valid RecordControl payload.");
            }
        }
        // 2. Handle Device Query Response (Server acknowledging/replying to device query)
        else if (protocol::IoT::client::isDeviceQueryResponse(pulled_result)) {
            protocol::IoT::BuisDeviceQueryResponse queryRes;
            
            if (protocol::IoT::client::decodeDeviceQueryResponse(pulled_result, queryRes)) {
                if (queryRes.status == protocol::IoT::Status_DeviceQuery::SUCCESS) {
                    ESP_LOGI("Server", "Device query acknowledged successfully. Message: %s", queryRes.message.c_str());
                } else {
                    ESP_LOGW("Server", "Device query rejected by server. Status code: %u, Message: %s", 
                             static_cast<uint8_t>(queryRes.status), queryRes.message.c_str());
                }
            } else {
                ESP_LOGW("Server", "Failed to decode BuisDeviceQueryResponse payload.");
            }
        }
        
        else {
            // Handle other incoming packet types or general server notifications here if needed
            ESP_LOGI("Server", "Received general server message or unhandled packet type.");
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