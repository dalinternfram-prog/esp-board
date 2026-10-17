// // #include <vector>
// // #include <span>
// // #include <cstdint>

// // /**
// //  * @brief Encodes a raw 16kHz interleaved stereo PCM audio frame into Opus format.
// //  * 
// //  * @param pcm_frame Span containing raw int16_t PCM samples (e.g. 1024 elements for 512 samples/ch).
// //  * @return std::vector<uint8_t> Compressed Opus payload (empty vector on encoding failure).
// //  */
// // std::vector<uint8_t> encodePcmToOpus(std::span<const int16_t> pcm_frame) ;
























// #pragma once

// #include "esp_audio_enc.h"
// #include "esp_opus_enc.h"
// #include "esp_err.h"

// #include <span>
// #include <vector>
// #include <cstdint>

// class OpusEncoder {
// public:
//     struct Config {
//         int sample_rate = 16000;                                   // 8000, 12000, 16000, 24000, 48000
//         int channels = 1;                                         // 1 (Mono) or 2 (Dual/Stereo)
//         int bits_per_sample = 16;                                 // Must be 16
//         int bitrate = 32000;                                      // Target bitrate in bps (e.g. 32000 for 32 kbps)
//         esp_opus_enc_frame_duration_t frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20_MS; // 2.5, 5, 10, 20, 40, 60, 80, 100, 120 ms
//         esp_opus_enc_application_t application_mode = ESP_OPUS_ENC_APPLICATION_VOIP;    // VOIP or AUDIO
//         int complexity = 5;                                       // 0 (lowest CPU) to 10 (highest quality)
//         bool enable_fec = false;                                  // Inband Forward Error Correction
//         bool enable_dtx = false;                                  // Discontinuous Transmission
//         bool enable_vbr = true;                                   // Variable Bitrate Mode
//     };

//     explicit OpusEncoder(const Config& config );
//     ~OpusEncoder();

//     // Prevent copying to manage encoder lifecycle safely
//     OpusEncoder(const OpusEncoder&) = delete;
//     OpusEncoder& operator=(const OpusEncoder&) = delete;

//     // Allow moving
//     OpusEncoder(OpusEncoder&& other) noexcept;
//     OpusEncoder& operator=(OpusEncoder&& other) noexcept;

//     /**
//      * @brief Initializes the underlying Opus encoder handle.
//      * @return ESP_OK on success, or Espressif audio error code on failure.
//      */
//     esp_err_t init();

//     /**
//      * @brief Closes and frees the encoder handle.
//      */
//     void deinit();

//     /**
//      * @brief Encodes raw PCM audio samples into an Opus packet using a pre-allocated buffer.
//      * 
//      * @param pcm_samples Span containing input 16-bit PCM samples.
//      * @param out_buffer Destination pointer for encoded Opus bytes.
//      * @param out_buffer_max_size Maximum capacity of output buffer in bytes.
//      * @return Number of encoded bytes on success, 0 on insufficient data, or -1 on error.
//      */
//     int encode(std::span<const int16_t> pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

//     /**
//      * @brief Encodes raw PCM audio samples into a heap-allocated std::vector payload.
//      * 
//      * @param pcm_samples Span containing input 16-bit PCM samples.
//      * @param max_output_size Max expected size for encoded frame (default 512 bytes).
//      * @return Vector containing encoded bytes, or empty vector on error/insufficient data.
//      */
//     std::vector<uint8_t> encode(std::span<const int16_t> pcm_samples, uint32_t max_output_size = 512);

//     /**
//      * @brief Returns whether the encoder is initialized and ready.
//      */
//     [[nodiscard]] bool isInitialized() const { return m_handle != nullptr; }

//     /**
//      * @brief Gets current configuration.
//      */
//     [[nodiscard]] const Config& getConfig() const { return m_config; }

// private:
//     Config m_config;
//     void* m_handle = nullptr;
// };











#pragma once

// Include Espressif audio codec headers for Opus initialization and encoding
#include "esp_audio_enc.h"
#include "esp_opus_enc.h"
#include "esp_err.h"

// Include standard C++ headers for memory spans, vectors, and types
#include <span>
#include <vector>
#include <cstdint>
#include <cstddef>

class SimpleOpusEncoder {
public:
    // Configuration structure containing only the parameters you need to provide
    struct Config {
        int sample_rate = 16000;  // Audio sampling rate in Hz (e.g., 16000 for 16kHz audio)
        int channels = 1;         // Number of audio channels (1 for Mono, 2 for Stereo)
        size_t chunk_size = 320;  // Total PCM samples per input frame (e.g., 320 samples)
    };

    // Constructor taking the essential configuration parameters
    explicit SimpleOpusEncoder(const Config& config);

    // Destructor to clean up resources automatically when the object goes out of scope
    ~SimpleOpusEncoder();

    // Prevent copy constructor to avoid duplicate pointer handles
    SimpleOpusEncoder(const SimpleOpusEncoder&) = delete;

    // Prevent copy assignment operator
    SimpleOpusEncoder& operator=(const SimpleOpusEncoder&) = delete;

    // Allow move constructor for transferring ownership of the encoder instance
    SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept;

    // Allow move assignment operator
    SimpleOpusEncoder& operator=(SimpleOpusEncoder&& other) noexcept;

    // Initializes the underlying Espressif Opus encoder handle
    esp_err_t init();

    // Frees the Opus encoder handle and releases allocated memory
    void deinit();

    // Encodes PCM samples using a user-provided pre-allocated buffer (Zero dynamic allocations)
    int encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

    // Encodes PCM samples and returns a standard vector containing the compressed Opus payload
    std::vector<uint8_t> encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size = 512);

    // Helper method to check if the internal handle is valid and ready
    [[nodiscard]] bool isInitialized() const { return m_handle != nullptr; }

    // Helper method to inspect the active configuration
    [[nodiscard]] const Config& getConfig() const { return m_config; }

private:
    // Internal helper function to convert sample count and rate into Opus frame duration
    esp_opus_enc_frame_duration_t calculateFrameDuration(int sample_rate, size_t chunk_size);

    Config m_config;         // Stores user-selected configuration settings
    void* m_handle = nullptr; // Pointer handle managing the underlying C encoder instance
};