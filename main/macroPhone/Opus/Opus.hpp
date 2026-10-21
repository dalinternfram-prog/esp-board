// // // // #include <vector>
// // // // #include <span>
// // // // #include <cstdint>

// // // // /**
// // // //  * @brief Encodes a raw 16kHz interleaved stereo PCM audio frame into Opus format.
// // // //  * 
// // // //  * @param pcm_frame Span containing raw int16_t PCM samples (e.g. 1024 elements for 512 samples/ch).
// // // //  * @return std::vector<uint8_t> Compressed Opus payload (empty vector on encoding failure).
// // // //  */
// // // // std::vector<uint8_t> encodePcmToOpus(std::span<const int16_t> pcm_frame) ;
























// // // #pragma once

// // // #include "esp_audio_enc.h"
// // // #include "esp_opus_enc.h"
// // // #include "esp_err.h"

// // // #include <span>
// // // #include <vector>
// // // #include <cstdint>

// // // class OpusEncoder {
// // // public:
// // //     struct Config {
// // //         int sample_rate = 16000;                                   // 8000, 12000, 16000, 24000, 48000
// // //         int channels = 1;                                         // 1 (Mono) or 2 (Dual/Stereo)
// // //         int bits_per_sample = 16;                                 // Must be 16
// // //         int bitrate = 32000;                                      // Target bitrate in bps (e.g. 32000 for 32 kbps)
// // //         esp_opus_enc_frame_duration_t frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20_MS; // 2.5, 5, 10, 20, 40, 60, 80, 100, 120 ms
// // //         esp_opus_enc_application_t application_mode = ESP_OPUS_ENC_APPLICATION_VOIP;    // VOIP or AUDIO
// // //         int complexity = 5;                                       // 0 (lowest CPU) to 10 (highest quality)
// // //         bool enable_fec = false;                                  // Inband Forward Error Correction
// // //         bool enable_dtx = false;                                  // Discontinuous Transmission
// // //         bool enable_vbr = true;                                   // Variable Bitrate Mode
// // //     };

// // //     explicit OpusEncoder(const Config& config );
// // //     ~OpusEncoder();

// // //     // Prevent copying to manage encoder lifecycle safely
// // //     OpusEncoder(const OpusEncoder&) = delete;
// // //     OpusEncoder& operator=(const OpusEncoder&) = delete;

// // //     // Allow moving
// // //     OpusEncoder(OpusEncoder&& other) noexcept;
// // //     OpusEncoder& operator=(OpusEncoder&& other) noexcept;

// // //     /**
// // //      * @brief Initializes the underlying Opus encoder handle.
// // //      * @return ESP_OK on success, or Espressif audio error code on failure.
// // //      */
// // //     esp_err_t init();

// // //     /**
// // //      * @brief Closes and frees the encoder handle.
// // //      */
// // //     void deinit();

// // //     /**
// // //      * @brief Encodes raw PCM audio samples into an Opus packet using a pre-allocated buffer.
// // //      * 
// // //      * @param pcm_samples Span containing input 16-bit PCM samples.
// // //      * @param out_buffer Destination pointer for encoded Opus bytes.
// // //      * @param out_buffer_max_size Maximum capacity of output buffer in bytes.
// // //      * @return Number of encoded bytes on success, 0 on insufficient data, or -1 on error.
// // //      */
// // //     int encode(std::span<const int16_t> pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

// // //     /**
// // //      * @brief Encodes raw PCM audio samples into a heap-allocated std::vector payload.
// // //      * 
// // //      * @param pcm_samples Span containing input 16-bit PCM samples.
// // //      * @param max_output_size Max expected size for encoded frame (default 512 bytes).
// // //      * @return Vector containing encoded bytes, or empty vector on error/insufficient data.
// // //      */
// // //     std::vector<uint8_t> encode(std::span<const int16_t> pcm_samples, uint32_t max_output_size = 512);

// // //     /**
// // //      * @brief Returns whether the encoder is initialized and ready.
// // //      */
// // //     [[nodiscard]] bool isInitialized() const { return m_handle != nullptr; }

// // //     /**
// // //      * @brief Gets current configuration.
// // //      */
// // //     [[nodiscard]] const Config& getConfig() const { return m_config; }

// // // private:
// // //     Config m_config;
// // //     void* m_handle = nullptr;
// // // };











// // #pragma once

// // // Include Espressif audio codec headers for Opus initialization and encoding
// // #include "esp_audio_enc.h"
// // #include "esp_opus_enc.h"
// // #include "esp_err.h"

// // // Include standard C++ headers for memory spans, vectors, and types
// // #include <span>
// // #include <vector>
// // #include <cstdint>
// // #include <cstddef>

// // class SimpleOpusEncoder {
// // public:
// //     // Configuration structure containing only the parameters you need to provide
// //     struct Config {
// //         int sample_rate = 16000;  // Audio sampling rate in Hz (e.g., 16000 for 16kHz audio)
// //         int channels = 1;         // Number of audio channels (1 for Mono, 2 for Stereo)
// //         size_t chunk_size = 320;  // Total PCM samples per input frame (e.g., 320 samples)
// //     };

// //     // Constructor taking the essential configuration parameters
// //     explicit SimpleOpusEncoder(const Config& config);

// //     // Destructor to clean up resources automatically when the object goes out of scope
// //     ~SimpleOpusEncoder();

// //     // Prevent copy constructor to avoid duplicate pointer handles
// //     SimpleOpusEncoder(const SimpleOpusEncoder&) = delete;

// //     // Prevent copy assignment operator
// //     SimpleOpusEncoder& operator=(const SimpleOpusEncoder&) = delete;

// //     // Allow move constructor for transferring ownership of the encoder instance
// //     SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept;

// //     // Allow move assignment operator
// //     SimpleOpusEncoder& operator=(SimpleOpusEncoder&& other) noexcept;

// //     // Initializes the underlying Espressif Opus encoder handle
// //     esp_err_t init();

// //     // Frees the Opus encoder handle and releases allocated memory
// //     void deinit();

// //     // Encodes PCM samples using a user-provided pre-allocated buffer (Zero dynamic allocations)
// //     int encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

// //     // Encodes PCM samples and returns a standard vector containing the compressed Opus payload
// //     std::vector<uint8_t> encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size = 512);

// //     // Helper method to check if the internal handle is valid and ready
// //     [[nodiscard]] bool isInitialized() const { return m_handle != nullptr; }

// //     // Helper method to inspect the active configuration
// //     [[nodiscard]] const Config& getConfig() const { return m_config; }

// // private:
// //     // Internal helper function to convert sample count and rate into Opus frame duration
// //     esp_opus_enc_frame_duration_t calculateFrameDuration(int sample_rate, size_t chunk_size);

// //     Config m_config;         // Stores user-selected configuration settings
// //     void* m_handle = nullptr; // Pointer handle managing the underlying C encoder instance
// // };











// #pragma once

// // Include Espressif audio codec headers for Opus initialization and encoding
// #include "esp_audio_enc.h"
// #include "esp_opus_enc.h"
// #include "esp_err.h"

// // Include standard C++ headers for memory spans, vectors, and types
// #include <span>
// #include <vector>
// #include <cstdint>
// #include <cstddef>

// class SimpleOpusEncoder {
// public:
//     // Configuration structure containing only the parameters you need to provide
//     struct Config {
//         int sample_rate = 16000;  // Audio sampling rate in Hz (e.g., 16000 for 16kHz audio)
//         int channels = 1;         // Number of audio channels (1 for Mono, 2 for Stereo)
//         // CHANGED: Removed intermediate chunk_size parameter to avoid middle-man configuration mismatches.
//     };

//     // Constructor taking the essential configuration parameters
//     explicit SimpleOpusEncoder(const Config& config);

//     // Destructor to clean up resources automatically when the object goes out of scope
//     ~SimpleOpusEncoder();

//     // Prevent copy constructor to avoid duplicate pointer handles
//     SimpleOpusEncoder(const SimpleOpusEncoder&) = delete;

//     // Prevent copy assignment operator
//     SimpleOpusEncoder& operator=(const SimpleOpusEncoder&) = delete;

//     // Allow move constructor for transferring ownership of the encoder instance
//     SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept;

//     // Allow move assignment operator
//     SimpleOpusEncoder& operator=(SimpleOpusEncoder&& other) noexcept;

//     // Initializes the underlying Espressif Opus encoder handle
//     // CHANGED: Added target frame size parameter so init knows what duration to configure.
//     esp_err_t init(size_t initial_chunk_size = 640);

//     // Frees the Opus encoder handle and releases allocated memory
//     void deinit();

//     // Encodes PCM samples using a user-provided pre-allocated buffer (Zero dynamic allocations)
//     int encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

//     // Encodes PCM samples and returns a standard vector containing the compressed Opus payload
//     std::vector<uint8_t> encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size = 512);

//     // Helper method to check if the internal handle is valid and ready
//     [[nodiscard]] bool isInitialized() const { return m_handle != nullptr; }

//     // Helper method to inspect the active configuration
//     [[nodiscard]] const Config& getConfig() const { return m_config; }

// private:
//     // Internal helper function to convert sample count and rate into Opus frame duration
//     esp_opus_enc_frame_duration_t calculateFrameDuration(int sample_rate, size_t chunk_size);

//     Config m_config;           // Stores user-selected configuration settings
//     void* m_handle = nullptr; // Pointer handle managing the underlying C encoder instance
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
    // Configuration structure containing only foundational parameters (No chunk size!)
    struct Config {
        int sample_rate = 16000;         // Audio sampling rate in Hz (e.g., 16000 for 16kHz audio)
        int channels = 1;                // Number of audio channels (1 for Mono, 2 for Stereo)
        uint32_t frame_duration_ms = 20; // Target frame duration in ms (e.g., 20ms standard for Opus)
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
    // CHANGED: Removed chunk size parameter entirely; initialization relies solely on Config.
    esp_err_t init();

    // Frees the Opus encoder handle and releases allocated memory
    void deinit();

    // Encodes PCM samples using a user-provided pre-allocated buffer (Zero dynamic allocations)
    int encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

    // Encodes PCM samples and returns a standard vector containing the compressed Opus payload
    std::vector<uint8_t> encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size = 1512);

    // Helper method to check if the internal handle is valid and ready
    [[nodiscard]] bool isInitialized() const { return m_handle != nullptr; }

    // Helper method to inspect the active configuration
    [[nodiscard]] const Config& getConfig() const { return m_config; }

private:
    // Internal helper function to map millisecond duration to Espressif API enum
    esp_opus_enc_frame_duration_t mapDurationToEnum(uint32_t duration_ms);
esp_opus_enc_config_t m_opus_cfg;
    Config m_config;           // Stores user-selected configuration settings
    void* m_handle = nullptr; // Pointer handle managing the underlying C encoder instance
};












// #pragma once

// #include "opus.h"
// #include "esp_err.h"
// #include <vector>
// #include <cstdint>
// #include <cstddef>

// class SimpleOpusEncoder {
// public:
//     // Configuration structure for the upstream libopus encoder
//     struct Config {
//         int sample_rate = 16000;         // Audio sampling rate in Hz (e.g., 16000)
//         int channels = 1;                // Number of audio channels (1 for Mono, 2 for Stereo)
//         uint32_t frame_duration_ms = 20; // Target frame duration in ms (20ms standard)
//         int bitrate = 24000;             // Compressed target bitrate in bps (e.g., 24kbps)
//         int complexity = 3;              // CPU complexity (0-10, lower is faster/optimized for ESP32-S3)
//     };

//     // Constructor taking the essential configuration parameters
//     explicit SimpleOpusEncoder(const Config& config);

//     // Destructor automatically cleans up native libopus encoder resources
//     ~SimpleOpusEncoder();

//     // Prevent copying to avoid duplicate pointer handles
//     SimpleOpusEncoder(const SimpleOpusEncoder&) = delete;
//     SimpleOpusEncoder& operator=(const SimpleOpusEncoder&) = delete;

//     // Allow move constructor and assignment
//     SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept;
//     SimpleOpusEncoder& operator=(SimpleOpusEncoder&& other) noexcept;

//     // Initializes the upstream Opus encoder instance
//     esp_err_t init();

//     // Frees the Opus encoder handle and releases memory
//     void deinit();

//     // Encodes PCM samples into a user-provided buffer (Zero-allocation path)
//     int encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

//     // Encodes PCM samples and returns a standard vector containing the compressed payload
//     std::vector<uint8_t> encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size = 1512);

//     // Helper method to check if the encoder is ready
//     [[nodiscard]] bool isInitialized() const { return m_encoder != nullptr; }

//     // Helper method to inspect active settings
//     [[nodiscard]] const Config& getConfig() const { return m_config; }

//     // Returns exact required sample count per channel per frame
//     [[nodiscard]] size_t getExpectedSamplesPerChannel() const { return m_expected_samples_per_channel; }

// private:
//     Config m_config;
//     OpusEncoder* m_encoder = nullptr;          // Native upstream libopus encoder state pointer
//     size_t m_expected_samples_per_channel = 0; // Calculated target samples per frame
// };



//  #pragma once

// // Include Espressif's native Opus decoder API header from the component registry
// extern "C" {
//     #include "esp_opus_dec.h"
// }

// #include "esp_err.h"   // Include ESP-IDF standard error type definitions
// #include <vector>      // Include standard vector container for PCM buffers
// #include <cstdint>     // Include standard fixed-width integer types (uint8_t, int16_t, etc.)
// #include <cstddef>     // Include standard definition macros (size_t)

// // Class wrapper providing a clean C++ object-oriented interface for Espressif's Opus decoder
// class SimpleOpusDecoder {
// public:
//     // Configuration structure matching Espressif's decoder requirements
//     struct Config {
//         uint32_t sample_rate = ESP_AUDIO_SAMPLE_RATE_16K; // Audio sampling rate (e.g., 16000 Hz)
//         uint8_t channel = ESP_AUDIO_MONO;                // Audio channels (ESP_AUDIO_MONO or ESP_AUDIO_DUAL)
//         esp_opus_dec_frame_duration_t frame_duration = ESP_OPUS_DEC_FRAME_DURATION_10_MS; // 10ms frame duration
//         bool self_delimited = false;                     // Stream packet delimiter flag (must match encoder)
//     };

//     // Explicit constructor initializing configuration settings
//     explicit SimpleOpusDecoder(const Config& config);

//     // Destructor automatically cleaning up decoder handles
//     ~SimpleOpusDecoder();

//     // Delete copy constructor to prevent sharing raw handle pointers
//     SimpleOpusDecoder(const SimpleOpusDecoder&) = delete;

//     // Delete copy assignment operator to prevent state corruption
//     SimpleOpusDecoder& operator=(const SimpleOpusDecoder&) = delete;

//     // Default move constructor enabling safe transfer of ownership across scopes
//     SimpleOpusDecoder(SimpleOpusDecoder&& other) noexcept;

//     // Default move assignment operator enabling resource re-assignment
//     SimpleOpusDecoder& operator=(SimpleOpusDecoder&& other) noexcept;

//     // Opens and initializes Espressif's native Opus decoder instance
//     esp_err_t init();

//     // Closes the decoder handle and frees internal allocations
//     void deinit();

//     // Decodes raw Opus byte packets into a vector of 16-bit PCM samples
//     std::vector<int16_t> decode(const uint8_t* opus_data, uint32_t opus_data_size);

//     // Inline helper method to check if the decoder is active and ready
//     [[nodiscard]] bool isInitialized() const { return m_dec_handle != nullptr; }

//     // Inline helper to inspect active configuration settings
//     [[nodiscard]] const Config& getConfig() const { return m_config; }

// private:
//     Config m_config;                // Stores active decoder configuration settings
//     void* m_dec_handle = nullptr;   // Opaque native decoder handle pointer used by Espressif APIs
// };














// #pragma once

// // Include Espressif's native Opus encoder API header from the component registry
// extern "C" {
//     #include "esp_opus_enc.h"
// }

// #include "esp_err.h"   // Include ESP-IDF standard error type definitions
// #include <vector>      // Include standard vector container for PCM/bitstream buffers
// #include <cstdint>     // Include standard fixed-width integer types (uint8_t, int16_t, etc.)
// #include <cstddef>     // Include standard definition macros (size_t)

// // Class wrapper providing a clean C++ object-oriented interface for Espressif's Opus encoder component
// class SimpleOpusEncoder {
// public:
//     // Configuration structure mapping to Espressif's encoder config requirements
//     struct Config {
//         uint32_t sample_rate = 16000;                     // Audio sampling rate in Hz (e.g., 16000)
//         uint8_t channel = ESP_AUDIO_MONO;                 // Audio channels (ESP_AUDIO_MONO or ESP_AUDIO_DUAL)
//         uint8_t bits_per_sample = 16;                     // Bits per PCM sample (typically 16)
//         int bitrate = 24000;                              // Target compressed bitrate in bits per second (bps)
//         esp_opus_enc_frame_duration_t frame_duration = ESP_OPUS_ENC_FRAME_DURATION_10_MS; // 10ms frame duration
//         int complexity = 3;                               // CPU complexity profile (0 to 10)
//     };

//     // Explicit constructor initializing configuration settings
//     explicit SimpleOpusEncoder(const Config& config);

//     // Destructor automatically cleaning up encoder handle allocations
//     ~SimpleOpusEncoder();

//     // Delete copy constructor to prevent sharing raw handle pointers
//     SimpleOpusEncoder(const SimpleOpusEncoder&) = delete;

//     // Delete copy assignment operator to prevent state corruption
//     SimpleOpusEncoder& operator=(const SimpleOpusEncoder&) = delete;

//     // Default move constructor enabling safe transfer of ownership across scopes
//     SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept;

//     // Default move assignment operator enabling resource re-assignment
//     SimpleOpusEncoder& operator=(SimpleOpusEncoder&& other) noexcept;

//     // Opens and initializes Espressif's native Opus encoder instance
//     esp_err_t init();

//     // Closes the encoder handle and frees internal allocations
//     void deinit();

//     // Encodes raw PCM samples into a user-provided output byte buffer
//     int encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

//     // Overloaded helper method encoding PCM samples and returning a dynamically sized vector
//     std::vector<uint8_t> encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size = 1512);

//     // Inline helper method to check if the encoder is active and ready
//     [[nodiscard]] bool isInitialized() const { return m_enc_handle != nullptr; }

//     // Inline helper to inspect active configuration settings
//     [[nodiscard]] const Config& getConfig() const { return m_config; }

// private:
//     Config m_config;                // Stores active encoder configuration settings
//     void* m_enc_handle = nullptr;   // Opaque native encoder handle pointer used by Espressif APIs
// };