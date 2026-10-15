// #pragma once

// #include "AudioTools.h" 
// #include "bsp_board.h"
// #include "driver/gpio.h"
// #include "esp_log.h"

// #include "AudioTools/Communication/UDPStream.h"
// class AudioStreamer {
// private:
//     static constexpr const char* TAG = "AUDIO_STREAMER";

//     I2SStream  m_i2s_mic;
//     UDPStream  m_udp_client;
//     StreamCopy m_copier;
//     bool       m_is_running{false};

// public:
//     AudioStreamer() = default;
//     ~AudioStreamer() = default;

//     /**
//      * @brief Initialize UDP audio streaming using board I2S microphone pins
//      * @param target_ip Destination IP address (e.g., "192.168.1.100")
//      * @param target_port Destination UDP port (e.g., 1234)
//      * @param sample_rate Audio sampling frequency in Hz (Default: 16000)
//      * @return true if initialization succeeded, false otherwise
//      */
//     bool begin(const char* target_ip, uint16_t target_port, uint32_t sample_rate = 16000);

//     /**
//      * @brief Pump audio data from the microphone to the UDP output
//      * @note Call this repeatedly inside a FreeRTOS task or main loop
//      */
//     void process();

//     /**
//      * @brief Resume audio processing/streaming
//      */
//     void start();

//     /**
//      * @brief Pause audio processing/streaming
//      */
//     void stop();

//     /**
//      * @brief Query current streaming state
//      * @return true if active, false if stopped
//      */
//     bool is_running() const;
// };