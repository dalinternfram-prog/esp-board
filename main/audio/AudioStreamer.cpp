// #include "AudioStreamer.hpp"

// bool AudioStreamer::begin(const char* target_ip, uint16_t target_port, uint32_t sample_rate) {
//     ESP_LOGI(TAG, "Initializing AudioStreamer...");

//     // 1. Configure Hardware I2S Microphone using hardcoded BSP board macros
//     auto mic_cfg = m_i2s_mic.defaultConfig(RX_MODE);
//     mic_cfg.sample_rate = sample_rate;
//     mic_cfg.channels = 1;
//     mic_cfg.bits_per_sample = 16;

//     // Hardcoded GPIO assignments from bsp_board.h
//     mic_cfg.pin_bck  = GPIO_I2S_SCLK; // GPIO 13
//     mic_cfg.pin_ws   = GPIO_I2S_LRCK; // GPIO 14
//     mic_cfg.pin_data = GPIO_I2S_SDIN; // GPIO 15

//     if (!m_i2s_mic.begin(mic_cfg)) {
//         ESP_LOGE(TAG, "Failed to initialize I2S Microphone");
//         return false;
//     }

//     // 2. Configure UDP Transport Output
//     if (!m_udp_client.begin(target_ip, target_port)) {
//         ESP_LOGE(TAG, "Failed to initialize UDP Client target %s:%d", target_ip, target_port);
//         return false;
//     }

//     // 3. Bind Stream Pipeline (Mic -> Copier -> UDP)
//     m_copier.begin(m_udp_client, m_i2s_mic);

//     m_is_running = true;
//     ESP_LOGI(TAG, "AudioStreamer initialized successfully targeting %s:%d", target_ip, target_port);
//     return true;
// }

// void AudioStreamer::process() {
//     if (m_is_running) {
//         m_copier.copy();
//     }
// }

// void AudioStreamer::start() {
//     m_is_running = true;
// }

// void AudioStreamer::stop() {
//     m_is_running = false;
// }

// bool AudioStreamer::is_running() const {
//     return m_is_running;
// }