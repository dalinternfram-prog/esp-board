#pragma once

#include "AudioTools.h"
#include "driver/gpio.h"
#include "esp_log.h"

class AudioStreamer {

    private:
    static constexpr const char* TAG = "AUDIO_STREAMER";

    I2SStream     m_i2s_mic;
    UDPStream     m_udp_client;
    StreamCopy    m_copier;
    bool          m_is_running{false};
public:
    AudioStreamer() = default;
    ~AudioStreamer() = default;

    // Initialize the microphone pins and UDP transport target
    bool begin(const char* target_ip, uint16_t target_port,
               gpio_num_t bck_pin, gpio_num_t ws_pin, gpio_num_t data_pin,
               uint32_t sample_rate = 16000) 
    {
        ESP_LOGI(TAG, "Initializing AudioStreamer...");

        // 1. Configure Hardware I2S Microphone
        auto mic_cfg = m_i2s_mic.defaultConfig(RX_MODE);
        mic_cfg.sample_rate = sample_rate;
        mic_cfg.channels = 1;
        mic_cfg.bits_per_sample = 16;
        mic_cfg.pin_bck = bck_pin;
        mic_cfg.pin_ws = ws_pin;
        mic_cfg.pin_data = data_pin;

        if (!m_i2s_mic.begin(mic_cfg)) {
            ESP_LOGE(TAG, "Failed to initialize I2S Microphone");
            return false;
        }

        // 2. Configure UDP Transport Output
        if (!m_udp_client.begin(target_ip, target_port)) {
            ESP_LOGE(TAG, "Failed to initialize UDP Client");
            return false;
        }

        // 3. Bind Stream Pipeline (Mic -> Copier -> UDP)
        m_copier.begin(m_udp_client, m_i2s_mic);

        m_is_running = true;
        ESP_LOGI(TAG, "AudioStreamer initialized successfully targeting %s:%d", target_ip, target_port);
        return true;
    }

    // Call this inside your loop or FreeRTOS task to pump audio data
    void process() {
        if (m_is_running) {
            m_copier.copy();
        }
    }

    // High-level controls
    void start() { m_is_running = true; }
    void stop()  { m_is_running = false; }
    bool is_running() const { return m_is_running; }


};