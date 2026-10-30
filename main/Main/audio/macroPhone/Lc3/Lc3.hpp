




// #pragma once

// // Include C headers wrapped for C++ compilation safety
// extern "C" {
//     #include "lc3.h" // Include the core Google liblc3 header
// }

// #include "esp_err.h"   // Include ESP-IDF error type definitions
// #include <vector>      // Include standard vector container for memory buffers
// #include <cstdint>     // Include standard fixed-width integer types (uint8_t, int16_t, etc.)
// #include <cstddef>     // Include standard definition macros (size_t)

// // Class wrapper providing a clean C++ object-oriented interface for Google liblc3 encoding
// class SimpleLC3Encoder {
// public:
//     // Configuration structure defining audio properties and encoder behavior
//     struct Config {
//         int sample_rate = 16000;           // Audio sampling rate in Hz (standard choices: 8000, 16000, 24000, 32000, 48000)
//         int channels = 1;                  // Number of audio channels (1 for Mono, 2 for Stereo)
//         uint32_t frame_duration_us = 10000;// Frame duration in microseconds (10000 us = 10 ms ultra-low latency)
//         int bitrate = 32000;               // Target compressed bitrate in bits per second (bps)
//         int epmr = 0;                      // Error Protection Mode Request (0 = disabled/default)
//     };

//     // Explicit constructor initializing configuration and setting defaults
//     explicit SimpleLC3Encoder(const Config& config);

//     // Destructor automatically cleaning up encoder memory allocations
//     ~SimpleLC3Encoder();

//     // Delete copy constructor to prevent shallow-copying raw pointer handles and memory blocks
//     SimpleLC3Encoder(const SimpleLC3Encoder&) = delete;

//     // Delete copy assignment operator to prevent shared state corruption
//     SimpleLC3Encoder& operator=(const SimpleLC3Encoder&) = delete;

//     // Default move constructor enabling safe transfer of ownership across scopes
//     SimpleLC3Encoder(SimpleLC3Encoder&& other) noexcept;

//     // Default move assignment operator enabling resource re-assignment
//     SimpleLC3Encoder& operator=(SimpleLC3Encoder&& other) noexcept;

//     // Allocates scratch memory and sets up the internal lc3 encoder instance
//     esp_err_t init();

//     // Frees encoder context and clears scratch buffer allocations
//     void deinit();

//     // Encodes a vector of raw PCM samples into a user-provided output byte buffer
//     int encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size);

//     // Overloaded helper method encoding PCM samples and returning a dynamically sized vector
//     std::vector<uint8_t> encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size = 400);

//     // Inline helper to check if the encoder has been successfully set up
//     [[nodiscard]] bool isInitialized() const { return m_encoder != nullptr; }

//     // Inline helper to retrieve active configuration parameters
//     [[nodiscard]] const Config& getConfig() const { return m_config; }

//     // Returns exact required sample count per channel per frame based on sample rate and duration
//     [[nodiscard]] size_t getExpectedSamplesPerChannel() const { return m_expected_samples_per_channel; }

// private:
//     Config m_config;                       // Stores active user configuration settings
//     std::vector<uint8_t> m_encoder_mem;    // Internal backing memory block required by liblc3
//     lc3_encoder_t m_encoder = nullptr;     // Opaque pointer representing the active LC3 encoder instance
//     size_t m_expected_samples_per_channel; // Computed exact count of PCM samples expected per frame channel
//     int m_target_bytes = 0;                // Computed target output byte size derived from bitrate and duration
// };