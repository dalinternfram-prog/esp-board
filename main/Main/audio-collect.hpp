#pragma once



  
#include<memory>
#include "Network/Net manager.hpp"


#include <iostream>

#include "audio_capture_driver.hpp"


#include<vector>

#include "Opus.hpp"
namespace Main::audio {







    class collector{





public:

bool Running;




void run(){
Running=true;



std::shared_ptr<SimpleOpusEncoder> s_SimpleOpusEncoder = std::make_shared<SimpleOpusEncoder>( );


std::shared_ptr<Hardware::AudioCaptureDriver> s_AudioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>( );


  

    while (Running) {
        // 2. Read new PCM frame from hardware
        esp_err_t ret = s_AudioCaptureDriver->readSamples();
        if (ret == ESP_OK) {
            
            std::vector<int16_t> audio = s_AudioCaptureDriver->getFrameBuffer();

  
 
        } 
        
        
        else {
            ESP_LOGW("++++++++++++", "Failed to read audio samples, retrying...");
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
    }



}




void stop(){
    Running=false;
}











    };
}