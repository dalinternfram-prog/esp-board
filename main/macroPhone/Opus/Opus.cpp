// // // #include <vector>
// // // #include <span>
// // // #include <cstdint>
// // // #include "esp_log.h"
// // // #include "esp_opus_enc.h"

// // // static const char* TAG = "OpusEncoder";

// // // /**
// // //  * @brief Encodes a raw 16kHz interleaved stereo PCM audio frame into Opus format.
// // //  * 
// // //  * @param pcm_frame Span containing raw int16_t PCM samples (e.g. 1024 elements for 512 samples/ch).
// // //  * @return std::vector<uint8_t> Compressed Opus payload (empty vector on encoding failure).
// // //  */
// // // std::vector<uint8_t> encodePcmToOpus(std::span<const int16_t> pcm_frame) {
// // //     static esp_opus_enc_handle_t s_opus_handle = nullptr;

// // //     // 1. One-time initialization of the Opus encoder handle
// // //     if (s_opus_handle == nullptr) {
// // //         esp_opus_enc_config_t opus_cfg = {
// // //             .sample_rate = 16000,
// // //             .channels = 2,                  // Interleaved Stereo
// // //             .bitrate = 32000,               // Target 32 kbps (ideal for 16kHz voice/audio)
// // //             .frame_size = 512,              // 512 samples per channel (32ms frame)
// // //             .application = OPUS_APPLICATION_VOIP,
// // //             .complexity = 5,                // Balanced CPU vs. quality for ESP32 (0–10)
// // //         };

// // //         esp_err_t err = esp_opus_enc_create(&opus_cfg, &s_opus_handle);
// // //         if (err != ESP_OK || s_opus_handle == nullptr) {
// // //             ESP_LOGE(TAG, "Failed to initialize Opus encoder instance: 0x%x", err);
// // //             return {};
// // //         }
// // //         ESP_LOGI(TAG, "Opus encoder initialized successfully (16kHz Stereo, 32kbps)");
// // //     }

// // //     if (pcm_frame.empty()) {
// // //         return {};
// // //     }

// // //     // 2. Allocate output buffer for encoded Opus packet
// // //     // Opus compressed output for a 32ms stereo frame at 32kbps is typically ~128 bytes
// // //     uint8_t encoded_buf[256];
// // //     int encoded_bytes = 0;

// // //     // 3. Perform fixed-point Opus encoding
// // //     esp_err_t ret = esp_opus_enc_process(
// // //         s_opus_handle, 
// // //         const_cast<int16_t*>(pcm_frame.data()), 
// // //         pcm_frame.size(),  // 1024 total elements
// // //         encoded_buf, 
// // //         &encoded_bytes
// // //     );

// // //     if (ret != ESP_OK || encoded_bytes <= 0) {
// // //         ESP_LOGW(TAG, "Opus frame encoding failed: 0x%x", ret);
// // //         return {};
// // //     }

// // //     // 4. Return compressed vector payload
// // //     return std::vector<uint8_t>(encoded_buf, encoded_buf + encoded_bytes);
// // // }



// // #include "esp_err.h"         // Fixes 'esp_err_t' and 'ESP_OK' errors
// // #include "esp_log.h"
// // #include "esp_opus_enc.h"    // Official Espressif Opus component header

// // #include <vector>
// // #include <span>
// // #include <cstdint>

// // static const char* TAG = "OpusEncoder";

// // std::vector<uint8_t> encodePcmToOpus(std::span<const int16_t> pcm_frame) {
// //     // Correct type definition: esp_opus_enc_handle_t is a pointer handle
// //     static esp_audio_enc_handle_t s_opus_handle = nullptr;

// //     // 1. One-time initialization of the Opus encoder handle
// //     if (s_opus_handle == nullptr) {
// //         // esp_opus_enc_config_t opus_cfg = {
// //         //     .sample_rate = 16000,
// //         //     .channel = 2,                              // Field name is 'channel' (singular)
// //         //     .bitrate = 32000,                          // 32 kbps target
// //         //     .frame_size = 512,                         // 512 samples per channel
// //         //     .application = ESP_OPUS_ENC_APPLICATION_VOIP, // Proper Espressif enum
// //         //     .complexity = 5,                           // Balanced CPU vs quality (0-10)
// //         // };
// // esp_opus_enc_config_t opus_cfg = {
// //     .sample_rate = 16000,
// //     .channel = 1,                  // 1 for mono, 2 for dual
// //     .bits_per_sample = 16,
// //     .bitrate = 32000,
// //     .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20_MS, // Use frame_duration, NOT frame_size
// //     .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,  // or ESP_OPUS_ENC_APPLICATION_AUDIO
// //     .complexity = 5,
// //     .enable_fec = false,
// //     .enable_dtx = false,
// //     .enable_vbr = true,
// // };
// // // 3. Call esp_opus_enc_open with 3 arguments
// // esp_audio_err_t err = esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &s_opus_handle);
// //      //   esp_err_t err = esp_opus_enc_open(&opus_cfg, &s_opus_handle);
// //         if (err != ESP_OK || s_opus_handle == nullptr) {
// //             ESP_LOGE(TAG, "Failed to initialize Opus encoder: 0x%x", err);
// //             return {};
// //         }
// //         ESP_LOGI(TAG, "Opus encoder initialized successfully");
// //     }

// //     if (pcm_frame.empty()) {
// //         return {};
// //     }

// //     // 2. Allocate output buffer for encoded payload
// //     uint8_t encoded_buf[256];
// //     int encoded_bytes = 0;


// //  // 1. Prepare input frame
// // esp_audio_enc_in_frame_t in_frame = {};
// // in_frame.buffer = reinterpret_cast<uint8_t*>(const_cast<int16_t*>(pcm_frame.data()));
// // in_frame.len = pcm_frame.size() * sizeof(int16_t); // Total size in bytes

// // // 2. Prepare output frame
// // esp_audio_enc_out_frame_t out_frame = {};
// // out_frame.buffer = reinterpret_cast<uint8_t*>(output_buffer);
// // out_frame.len = output_buffer_max_size; // Allocated capacity in bytes
// // out_frame.encoded_bytes = 0;             // Will be populated by encoder
// // out_frame.pts = 0;

// // // 3. Process frame
// // esp_audio_err_t err = esp_opus_enc_process(s_opus_handle, &in_frame, &out_frame);

// // if (err == ESP_AUDIO_ERR_OK) {
// //     // Read actual encoded size from encoded_bytes field
// //     uint32_t encoded_len = out_frame.encoded_bytes;
// //     uint64_t frame_pts = out_frame.pts;
// // } else if (err == ESP_AUDIO_ERR_DATA_LACK) {
// //     // Input PCM length was smaller than required frame size
// // } else {
// //     // Encoding error
// // }



// //     // 3. Perform fixed-point Opus encoding
// //     esp_err_t ret = esp_opus_enc_process(
// //         s_opus_handle, 
// //         const_cast<int16_t*>(pcm_frame.data()), 
// //         pcm_frame.size(), 
// //         encoded_buf, 
// //         &encoded_bytes
// //     );

// //     if (ret != ESP_OK || encoded_bytes <= 0) {
// //         ESP_LOGW(TAG, "Opus frame encoding failed: 0x%x", ret);
// //         return {};
// //     }

// //     return std::vector<uint8_t>(encoded_buf, encoded_buf + encoded_bytes);
// // }







// // //======================================
// // //======================================
// // //======================================
// // //======================================






// // #include "Opus.hpp"
// // #include "esp_opus_enc.h"
// // #include "esp_audio_enc.h"
// // #include "esp_log.h"
// // #include <vector>

// // static const char *TAG = "OPUS_ENC";

// // // CHANGE: Changed handle type to void* to match esp_opus_enc_open signature
// // static void *s_opus_handle = nullptr;

// // esp_err_t init_opus_encoder(void) {
// //     if (s_opus_handle != nullptr) {
// //         ESP_LOGW(TAG, "Opus encoder already initialized");
// //         return ESP_OK;
// //     }

// //     // CHANGE: Updated struct fields to match esp_opus_enc_config_t definition:
// //     // - Used .frame_duration instead of non-existent .frame_size
// //     // - Added missing parameters (.bits_per_sample, .application_mode, .complexity, etc.)
// //     esp_opus_enc_config_t opus_cfg = {
// //         .sample_rate = 16000,
// //         .channel = 1,                                       // 1 for Mono, 2 for Dual
// //         .bits_per_sample = 16,                             // Must be 16
// //         .bitrate = 32000,                                   // 32 kbps
// //         .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20MS, // Replaced frame_size
// //         .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,  // VOIP or AUDIO
// //         .complexity = 5,                                    // 0 (lowest) to 10 (highest)
// //         .enable_fec = false,
// //         .enable_dtx = false,
// //         .enable_vbr = true,
// //     };

// //     // CHANGE: Replaced non-existent 'esp_opus_enc_create' with 'esp_opus_enc_open'
// //     // Passed sizeof(esp_opus_enc_config_t) as required by the 3-argument signature
// //     esp_audio_err_t err = esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &s_opus_handle);
// //     if (err != ESP_AUDIO_ERR_OK) {
// //         ESP_LOGE(TAG, "Failed to initialize Opus encoder: 0x%x", err);
// //         s_opus_handle = nullptr;
// //         return ESP_FAIL;
// //     }

// //     ESP_LOGI(TAG, "Opus encoder initialized successfully");
// //     return ESP_OK;
// // }

// // int encode_opus_frame(const std::vector<int16_t> &pcm_frame, uint8_t *output_buffer, uint32_t output_buffer_max_size) {
// //     if (s_opus_handle == nullptr) {
// //         ESP_LOGE(TAG, "Encoder handle is null");
// //         return -1;
// //     }

// //     // CHANGE: Wrapped raw int16_t* PCM buffer inside esp_audio_enc_in_frame_t
// //     // Used reinterpret_cast to match uint8_t* expected by the struct definition
// //     // Converted sample count to byte count (samples * sizeof(int16_t))
// //     esp_audio_enc_in_frame_t in_frame = {};
// //     in_frame.buffer = reinterpret_cast<uint8_t*>(const_cast<int16_t*>(pcm_frame.data()));
// //     in_frame.len = pcm_frame.size() * sizeof(int16_t);

// //     // CHANGE: Prepared esp_audio_enc_out_frame_t with allocated capacity
// //     esp_audio_enc_out_frame_t out_frame = {};
// //     out_frame.buffer = output_buffer;
// //     out_frame.len = output_buffer_max_size;
// //     out_frame.encoded_bytes = 0;
// //     out_frame.pts = 0;

// //     // CHANGE: Passed struct pointers to esp_opus_enc_process instead of raw data pointers
// //     esp_audio_err_t err = esp_opus_enc_process(s_opus_handle, &in_frame, &out_frame);

// //     if (err == ESP_AUDIO_ERR_OK) {
// //         // CHANGE: Retrieved encoded size from out_frame.encoded_bytes instead of return value
// //         return out_frame.encoded_bytes;
// //     } else if (err == ESP_AUDIO_ERR_DATA_LACK) {
// //         ESP_LOGW(TAG, "Insufficient PCM data supplied for a full frame");
// //         return 0;
// //     } else {
// //         ESP_LOGE(TAG, "Encoding failed with error code: 0x%x", err);
// //         return -1;
// //     }
// // }

// // void deinit_opus_encoder(void) {
// //     if (s_opus_handle != nullptr) {
// //         // CHANGE: Call close to release allocated memory
// //         esp_opus_enc_close(s_opus_handle);
// //         s_opus_handle = nullptr;
// //         ESP_LOGI(TAG, "Opus encoder closed");
// //     }
// // }
// #include "./Opus.hpp"
// #include "esp_log.h"
// #include <utility>

// static const char* TAG = "OpusEncoder";

// OpusEncoder::OpusEncoder(const Config& config)
//     : m_config(config), m_handle(nullptr) {}

// OpusEncoder::~OpusEncoder() {
//     deinit();
// }

// OpusEncoder::OpusEncoder(OpusEncoder&& other) noexcept
//     : m_config(other.m_config), m_handle(other.m_handle) {
//     other.m_handle = nullptr;
// }

// OpusEncoder& OpusEncoder::operator=(OpusEncoder&& other) noexcept {
//     if (this != &other) {
//         deinit();
//         m_config = other.m_config;
//         m_handle = other.m_handle;
//         other.m_handle = nullptr;
//     }
//     return *this;
// }

// esp_err_t OpusEncoder::init() {
//     if (m_handle != nullptr) {
//         ESP_LOGW(TAG, "Encoder already initialized");
//         return ESP_OK;
//     }

//     esp_opus_enc_config_t opus_cfg = {
//         .sample_rate = m_config.sample_rate,
//         .channel = m_config.channels,
//         .bits_per_sample = m_config.bits_per_sample,
//         .bitrate = m_config.bitrate,
//         .frame_duration = m_config.frame_duration,
//         .application_mode = m_config.application_mode,
//         .complexity = m_config.complexity,
//         .enable_fec = m_config.enable_fec,
//         .enable_dtx = m_config.enable_dtx,
//         .enable_vbr = m_config.enable_vbr,
//     };

//     esp_audio_err_t err = esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &m_handle);
//     if (err != ESP_AUDIO_ERR_OK || m_handle == nullptr) {
//         ESP_LOGE(TAG, "Failed to initialize Opus encoder: 0x%x", err);
//         m_handle = nullptr;
//         return ESP_FAIL;
//     }

//     ESP_LOGI(TAG, "Opus encoder initialized (%d Hz, %d ch, %d bps)", 
//              m_config.sample_rate, m_config.channels, m_config.bitrate);
//     return ESP_OK;
// }

// void OpusEncoder::deinit() {
//     if (m_handle != nullptr) {
//         esp_opus_enc_close(m_handle);
//         m_handle = nullptr;
//         ESP_LOGI(TAG, "Opus encoder closed");
//     }
// }

// int OpusEncoder::encode(std::span<const int16_t> pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size) {
//     if (m_handle == nullptr) {
//         ESP_LOGE(TAG, "Encoder handle is uninitialized");
//         return -1;
//     }

//     if (pcm_samples.empty() || out_buffer == nullptr || out_buffer_max_size == 0) {
//         return 0;
//     }

//     esp_audio_enc_in_frame_t in_frame = {};
//     in_frame.buffer = reinterpret_cast<uint8_t*>(const_cast<int16_t*>(pcm_samples.data()));
//     in_frame.len = pcm_samples.size() * sizeof(int16_t);

//     esp_audio_enc_out_frame_t out_frame = {};
//     out_frame.buffer = out_buffer;
//     out_frame.len = out_buffer_max_size;
//     out_frame.encoded_bytes = 0;
//     out_frame.pts = 0;

//     esp_audio_err_t err = esp_opus_enc_process(m_handle, &in_frame, &out_frame);

//     if (err == ESP_AUDIO_ERR_OK) {
//         return static_cast<int>(out_frame.encoded_bytes);
//     } else if (err == ESP_AUDIO_ERR_DATA_LACK) {
//         ESP_LOGW(TAG, "Insufficient PCM data provided for a full Opus frame");
//         return 0;
//     } else {
//         ESP_LOGE(TAG, "Opus process failed with error code: 0x%x", err);
//         return -1;
//     }
// }

// std::vector<uint8_t> OpusEncoder::encode(std::span<const int16_t> pcm_samples, uint32_t max_output_size) {
//     std::vector<uint8_t> output_vec(max_output_size);
//     int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);

//     if (bytes_written > 0) {
//         output_vec.resize(bytes_written);
//         return output_vec;
//     }

//     return {};
// }











#include "Opus.hpp"
#include "esp_log.h"

// Tag string used for ESP_LOG error and information logging
static const char* TAG = "SimpleOpusEncoder";

// Constructor: Initializes internal configuration and sets handle pointer to null
SimpleOpusEncoder::SimpleOpusEncoder(const Config& config)
    : m_config(config), m_handle(nullptr) {}

// Destructor: Automatically calls deinit to release hardware resources
SimpleOpusEncoder::~SimpleOpusEncoder() {
    deinit();
}

// Move Constructor: Transfers handle ownership from another instance
SimpleOpusEncoder::SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept
    : m_config(other.m_config), m_handle(other.m_handle) {
    other.m_handle = nullptr; // Nullify original handle so it isn't closed by destructor
}

// Move Assignment Operator: Cleans up existing state and takes handle from another instance
SimpleOpusEncoder& SimpleOpusEncoder::operator=(SimpleOpusEncoder&& other) noexcept {
    if (this != &other) {          // Guard against self-assignment
        deinit();                  // Free current resources
        m_config = other.m_config; // Copy configuration
        m_handle = other.m_handle; // Transfer handle pointer
        other.m_handle = nullptr;  // Invalidate source handle
    }
    return *this;
}

// Helper method: Calculates the required Opus enum from sample rate and PCM sample count
esp_opus_enc_frame_duration_t SimpleOpusEncoder::calculateFrameDuration(int sample_rate, size_t chunk_size) {
    // For stereo, samples are interleaved, so calculate samples per individual channel
    size_t samples_per_channel = (m_config.channels > 1) ? (chunk_size / m_config.channels) : chunk_size;

    // Formula: duration_in_ms = (samples_per_channel * 1000) / sample_rate
    uint32_t duration_ms = static_cast<uint32_t>((samples_per_channel * 1000) / sample_rate);

    // Match calculated millisecond duration to the corresponding Espressif API enum
    switch (duration_ms) {
        case 2:  return ESP_OPUS_ENC_FRAME_DURATION_2_5_MS;  // 2.5ms frame duration
        case 5:  return ESP_OPUS_ENC_FRAME_DURATION_5_MS;    // 5ms frame duration
        case 10: return ESP_OPUS_ENC_FRAME_DURATION_10_MS;   // 10ms frame duration
        case 20: return ESP_OPUS_ENC_FRAME_DURATION_20_MS;   // 20ms frame duration (Standard)
        case 40: return ESP_OPUS_ENC_FRAME_DURATION_40_MS;   // 40ms frame duration
        case 60: return ESP_OPUS_ENC_FRAME_DURATION_60_MS;   // 60ms frame duration
        default:
            // Log warning if duration is non-standard and fall back to the safe 20ms default
            ESP_LOGW(TAG, "Unusual frame duration (%lu ms), defaulting to 20ms", duration_ms);
            return ESP_OPUS_ENC_FRAME_DURATION_20_MS;
    }
}

// Opens and configures the Espressif Opus encoder handle
esp_err_t SimpleOpusEncoder::init() {
    // Return immediately if the encoder is already initialized
    if (m_handle != nullptr) {
        ESP_LOGW(TAG, "Encoder handle already initialized");
        return ESP_OK;
    }

    // Set standard target bitrate based on sampling rate (32 kbps for <=16kHz, 64 kbps for >16kHz)
    int default_bitrate = (m_config.sample_rate >= 24000) ? 64000 : 32000;

    // Populate Espressif's underlying opus configuration structure
    esp_opus_enc_config_t opus_cfg = {
        .sample_rate = m_config.sample_rate,                                                // Audio sample rate
        .channel = m_config.channels,                                                      // Channel count (1 or 2)
        .bits_per_sample = 16,                                                             // Fixed at 16-bit PCM
        .bitrate = default_bitrate,                                                        // Compressed target bitrate in bps
        .frame_duration = calculateFrameDuration(m_config.sample_rate, m_config.chunk_size), // Frame duration enum
        .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,                                 // VOIP mode optimized for voice
        .complexity = 5,                                                                   // ESP32 optimal CPU complexity balance (0-10)
        .enable_fec = false,                                                               // Disable Forward Error Correction
        .enable_dtx = false,                                                               // Disable Discontinuous Transmission
        .enable_vbr = true,                                                                // Enable Variable Bitrate mode
    };

    // Open the Opus encoder instance passing configuration, structure size, and handle reference pointer
    esp_audio_err_t err = esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &m_handle);

    // Verify if initialization was successful and handle was populated
    if (err != ESP_AUDIO_ERR_OK || m_handle == nullptr) {
        ESP_LOGE(TAG, "esp_opus_enc_open failed with error code: 0x%x", err);
        m_handle = nullptr; // Ensure handle remains null on error
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Encoder ready (%d Hz, %d ch, chunk=%u)", m_config.sample_rate, m_config.channels, (unsigned)m_config.chunk_size);
    return ESP_OK;
}

// Closes and frees the encoder memory
void SimpleOpusEncoder::deinit() {
    // Only attempt cleanup if handle is valid
    if (m_handle != nullptr) {
        esp_opus_enc_close(m_handle); // Call Espressif close function
        m_handle = nullptr;           // Reset pointer handle to null
        ESP_LOGI(TAG, "Opus encoder successfully closed");
    }
}

// Main encoding function: processes PCM audio into a raw buffer
int SimpleOpusEncoder::encode(std::span<const int16_t> pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size) {
    // Validate that encoder is initialized
    if (m_handle == nullptr) {
        ESP_LOGE(TAG, "Encoder is uninitialized. Call init() first.");
        return -1;
    }

    // Validate input data parameters
    if (pcm_samples.empty() || out_buffer == nullptr || out_buffer_max_size == 0) {
        return 0;
    }

    // Prepare input frame structure expected by Espressif's audio process API
    esp_audio_enc_in_frame_t in_frame = {};
    // Reinterpret signed 16-bit PCM pointer as unsigned byte pointer
    in_frame.buffer = reinterpret_cast<uint8_t*>(const_cast<int16_t*>(pcm_samples.data()));
    // Set total length of input data in bytes (sample_count * 2)
    in_frame.len = pcm_samples.size() * sizeof(int16_t);

    // Prepare output frame structure to hold compressed packet data
    esp_audio_enc_out_frame_t out_frame = {};
    out_frame.buffer = out_buffer;           // Point to destination output memory
    out_frame.len = out_buffer_max_size;    // Pass max available capacity of destination buffer
    out_frame.encoded_bytes = 0;            // Initialize output size accumulator to zero
    out_frame.pts = 0;                      // Initialize presentation timestamp to zero

    // Perform actual Opus compression encoding
    esp_audio_err_t err = esp_opus_enc_process(m_handle, &in_frame, &out_frame);

    // Evaluate encoding result
    if (err == ESP_AUDIO_ERR_OK) {
        // Success: Return number of bytes written to output buffer
        return static_cast<int>(out_frame.encoded_bytes);
    } else if (err == ESP_AUDIO_ERR_DATA_LACK) {
        // Warning: Input PCM sample count was lower than required frame duration
        ESP_LOGW(TAG, "Insufficient PCM samples supplied for one Opus frame");
        return 0;
    } else {
        // Error: Encoding pipeline failed
        ESP_LOGE(TAG, "esp_opus_enc_process error: 0x%x", err);
        return -1;
    }
}

// Convenient overload function: returns compressed payload inside a standard std::vector
std::vector<uint8_t> SimpleOpusEncoder::encode(std::span<const int16_t> pcm_samples, uint32_t max_output_size) {
    // Allocate temporary vector with maximum allocated capacity
    std::vector<uint8_t> output_vec(max_output_size);

    // Call raw buffer encoding method passing vector buffer memory
    int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);

    // Check if encoding produced output
    if (bytes_written > 0) {
        // Shrink vector down to match exact encoded byte length
        output_vec.resize(bytes_written);
        return output_vec;
    }

    // Return empty vector on failure or missing samples
    return {};
}