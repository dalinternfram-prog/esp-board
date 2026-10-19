// #pragma once



  
// #include<memory>
// #include "Network/Net manager.hpp"


// #include <iostream>

// #include "audio_capture_driver.hpp"


// #include<vector>

// #include "Opus.hpp"
// #include "macroPhone/arguments.hpp"
// namespace Main::audio {







//     class collector{





// public:

// bool Running;




// void run(std::shared_ptr<Network::manager>s_NetworkManager){
// Running=true;



// std::shared_ptr<SimpleOpusEncoder> s_SimpleOpusEncoder = std::make_shared<SimpleOpusEncoder>( 
//     SimpleOpusEncoder::Config{macroPhone::argument::sample_rate,macroPhone::argument::channel_count,macroPhone::argument::kDefaultChunkSize});

 
// std::shared_ptr<Hardware::AudioCaptureDriver> s_AudioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>( );


  

//     while (Running) {
//         // 2. Read new PCM frame from hardware
//         esp_err_t ret = s_AudioCaptureDriver->readSamples();
//         if (ret == ESP_OK) {
            
//             std::vector<int16_t> audio = s_AudioCaptureDriver->getFrameBuffer();

//     std::vector<uint8_t> compressed_ = s_SimpleOpusEncoder->encode(audio);

 
//         } 
        
        
//         else {
//             ESP_LOGW("++++++++++++", "Failed to read audio samples, retrying...");
//         }

//         vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
//     }



// }




// void stop(){
//     Running=false;
// }











//     };
// }