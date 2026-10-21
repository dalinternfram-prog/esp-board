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











// #include "Opus.hpp"
// #include "esp_log.h"

// // Tag string used for ESP_LOG error and information logging
// static const char* TAG = "SimpleOpusEncoder";

// // Constructor: Initializes internal configuration and sets handle pointer to null
// SimpleOpusEncoder::SimpleOpusEncoder(const Config& config)
//     : m_config(config), m_handle(nullptr) {}

// // Destructor: Automatically calls deinit to release hardware resources
// SimpleOpusEncoder::~SimpleOpusEncoder() {
//     deinit();
// }

// // Move Constructor: Transfers handle ownership from another instance
// SimpleOpusEncoder::SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept
//     : m_config(other.m_config), m_handle(other.m_handle) {
//     other.m_handle = nullptr; // Nullify original handle so it isn't closed by destructor
// }

// // Move Assignment Operator: Cleans up existing state and takes handle from another instance
// SimpleOpusEncoder& SimpleOpusEncoder::operator=(SimpleOpusEncoder&& other) noexcept {
//     if (this != &other) {          // Guard against self-assignment
//         deinit();                  // Free current resources
//         m_config = other.m_config; // Copy configuration
//         m_handle = other.m_handle; // Transfer handle pointer
//         other.m_handle = nullptr;  // Invalidate source handle
//     }
//     return *this;
// }

// // Helper method: Calculates the required Opus enum from sample rate and PCM sample count
// esp_opus_enc_frame_duration_t SimpleOpusEncoder::calculateFrameDuration(int sample_rate, size_t chunk_size) {
//     // For stereo, samples are interleaved, so calculate samples per individual channel
//     size_t samples_per_channel = (m_config.channels > 1) ? (chunk_size / m_config.channels) : chunk_size;

//     // Formula: duration_in_ms = (samples_per_channel * 1000) / sample_rate
//     uint32_t duration_ms = static_cast<uint32_t>((samples_per_channel * 1000) / sample_rate);
//    ESP_LOGI("++++++++++++++++++=", "duration = %u,chunk size %zu",duration_ms,chunk_size);
//     // Match calculated millisecond duration to the corresponding Espressif API enum
//     switch (duration_ms) {
//         case 2:  return ESP_OPUS_ENC_FRAME_DURATION_2_5_MS;  // 2.5ms frame duration
//         case 5:  return ESP_OPUS_ENC_FRAME_DURATION_5_MS;    // 5ms frame duration
//         case 10: return ESP_OPUS_ENC_FRAME_DURATION_10_MS;   // 10ms frame duration
//         case 20: return ESP_OPUS_ENC_FRAME_DURATION_20_MS;   // 20ms frame duration (Standard)
//         case 40: return ESP_OPUS_ENC_FRAME_DURATION_40_MS;   // 40ms frame duration
//         case 60: return ESP_OPUS_ENC_FRAME_DURATION_60_MS;   // 60ms frame duration
//         default:
//             // Log warning if duration is non-standard and fall back to the safe 20ms default
//             ESP_LOGW(TAG, "Unusual frame duration (%lu ms), defaulting to 20ms", duration_ms);
//             return ESP_OPUS_ENC_FRAME_DURATION_20_MS;
//     }
// }

// // Opens and configures the Espressif Opus encoder handle
// esp_err_t SimpleOpusEncoder::init() {
//     // Return immediately if the encoder is already initialized
//     if (m_handle != nullptr) {
//         ESP_LOGW(TAG, "Encoder handle already initialized");
//         return ESP_OK;
//     }

//     // Set standard target bitrate based on sampling rate (32 kbps for <=16kHz, 64 kbps for >16kHz)
//     int default_bitrate = (m_config.sample_rate >= 24000) ? 64000 : 32000;

//     // Populate Espressif's underlying opus configuration structure
//     esp_opus_enc_config_t opus_cfg = {
//         .sample_rate = m_config.sample_rate,                                                // Audio sample rate
//         .channel = m_config.channels,                                                      // Channel count (1 or 2)
//         .bits_per_sample = 16,                                                             // Fixed at 16-bit PCM
//         .bitrate = default_bitrate,                                                        // Compressed target bitrate in bps
//         .frame_duration = calculateFrameDuration(m_config.sample_rate, m_config.chunk_size), // Frame duration enum
//         .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,                                 // VOIP mode optimized for voice
//         .complexity = 5,                                                                   // ESP32 optimal CPU complexity balance (0-10)
//         .enable_fec = false,                                                               // Disable Forward Error Correction
//         .enable_dtx = false,                                                               // Disable Discontinuous Transmission
//         .enable_vbr = true,                                                                // Enable Variable Bitrate mode
//     };

//     // Open the Opus encoder instance passing configuration, structure size, and handle reference pointer
//     esp_audio_err_t err = esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &m_handle);

//     // Verify if initialization was successful and handle was populated
//     if (err != ESP_AUDIO_ERR_OK || m_handle == nullptr) {
//         ESP_LOGE(TAG, "esp_opus_enc_open failed with error code: 0x%x", err);
//         m_handle = nullptr; // Ensure handle remains null on error
//         return ESP_FAIL;
//     }

//     ESP_LOGI(TAG, "Encoder ready (%d Hz, %d ch, chunk=%u)", m_config.sample_rate, m_config.channels, (unsigned)m_config.chunk_size);
//     return ESP_OK;
// }

// // Closes and frees the encoder memory
// void SimpleOpusEncoder::deinit() {
//     // Only attempt cleanup if handle is valid
//     if (m_handle != nullptr) {
//         esp_opus_enc_close(m_handle); // Call Espressif close function
//         m_handle = nullptr;           // Reset pointer handle to null
//         ESP_LOGI(TAG, "Opus encoder successfully closed");
//     }
// }

//  // Main encoding function: processes PCM audio into a raw buffer
// // CHANGED: Parameter type changed from std::span<const int16_t> to const std::vector<int16_t>&
// int SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size) {
//     // Validate that encoder is initialized
//     if (m_handle == nullptr) {
//         ESP_LOGE(TAG, "Encoder is uninitialized. Call init() first.");
//         return -1;
//     }

//     // Validate input data parameters
//     if (pcm_samples.empty() || out_buffer == nullptr || out_buffer_max_size == 0) {
//         return 0;
//     }

//     // Prepare input frame structure expected by Espressif's audio process API
//     esp_audio_enc_in_frame_t in_frame = {};
    
//     // Reinterpret signed 16-bit PCM pointer as unsigned byte pointer
//     // CHANGED: Safe conversion from const vector pointer to uint8_t* to avoid modifying underlying data
//     in_frame.buffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(pcm_samples.data()));
    
//     // Set total length of input data in bytes (sample_count * 2)
//     // CHANGED: Uses vector size * sizeof(int16_t)
//     in_frame.len = static_cast<uint32_t>(pcm_samples.size() * sizeof(int16_t));

//     // Prepare output frame structure to hold compressed packet data
//     esp_audio_enc_out_frame_t out_frame = {};
//     out_frame.buffer = out_buffer;           // Point to destination output memory
//     out_frame.len = out_buffer_max_size;    // Pass max available capacity of destination buffer
//     out_frame.encoded_bytes = 0;            // Initialize output size accumulator to zero
//     out_frame.pts = 0;                      // Initialize presentation timestamp to zero

//     // Perform actual Opus compression encoding

// ESP_LOGI(TAG, "Encoding check: pcm_samples.size() = %u, out_buffer_max_size = %u", 
//          (unsigned int)pcm_samples.size(), (unsigned int)out_buffer_max_size);

// if (pcm_samples.size() != 640) { // Adjust 640 to your expected chunk size
//     ESP_LOGE(TAG, "CRITICAL: PCM sample count mismatch! Expected 640, got %u", (unsigned int)pcm_samples.size());
// }
//             ESP_LOGI("++++++++++++", "before esp_opus_enc_process  ");
//     esp_audio_err_t err = esp_opus_enc_process(m_handle, &in_frame, &out_frame);

//             ESP_LOGI("++++++++++++", "success esp_opus_enc_process  ");
//     // Evaluate encoding result
//     if (err == ESP_AUDIO_ERR_OK) {
//         // Success: Return number of bytes written to output buffer
//         return static_cast<int>(out_frame.encoded_bytes);
//     } else if (err == ESP_AUDIO_ERR_DATA_LACK) {
//         // Warning: Input PCM sample count was lower than required frame duration
//         ESP_LOGW(TAG, "Insufficient PCM samples supplied for one Opus frame");
//         return 0;
//     } else {
//         // Error: Encoding pipeline failed
//         ESP_LOGE(TAG, "esp_opus_enc_process error: 0x%x", err);
//         return -1;
//     }
// }

// // Convenient overload function: returns compressed payload inside a standard std::vector
// // CHANGED: Parameter type changed from std::span<const int16_t> to const std::vector<int16_t>&
// std::vector<uint8_t> SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size) {
//     // Allocate temporary vector with maximum allocated capacity

//             ESP_LOGI("++++++++++++", "before  std::vector<uint8_t> output_vec(max_output_size);");
//     std::vector<uint8_t> output_vec(max_output_size);

       
//     // Call raw buffer encoding method passing vector buffer memory
//     int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);
//      ESP_LOGI("++++++++++++", "success    int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);");
//     // Check if encoding produced output
//     if (bytes_written > 0) {
//         // Shrink vector down to match exact encoded byte length
//         output_vec.resize(bytes_written);
//         return output_vec;
//     }

//     // Return empty vector on failure or missing samples
//     return {};
// }
















































#include "Opus.hpp"
#include "esp_log.h"
 #include "esp_audio_enc_default.h" 
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
    if (this != &other) {         // Guard against self-assignment
        deinit();                 // Free current resources
        m_config = other.m_config; // Copy configuration
        m_handle = other.m_handle; // Transfer handle pointer
        other.m_handle = nullptr;  // Invalidate source handle
    }
    return *this;
}

// Helper method: Maps millisecond duration directly to Espressif API enum
esp_opus_enc_frame_duration_t SimpleOpusEncoder::mapDurationToEnum(uint32_t duration_ms) {
    ESP_LOGI("++++++++++++++++++=", "Target frame duration = %u ms", duration_ms);
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
   m_opus_cfg = {
        .sample_rate = m_config.sample_rate,                                                // Audio sample rate
        .channel = m_config.channels,                                                      // Channel count (1 or 2)
        .bits_per_sample = 16,                                                             // Fixed at 16-bit PCM
        .bitrate = default_bitrate,                                                        // Compressed target bitrate in bps
        .frame_duration = mapDurationToEnum(m_config.frame_duration_ms),                   // CHANGED: Derived directly from config duration instead of chunk size
        .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,                                 // VOIP mode optimized for voice
        .complexity = 5,                                                                   // ESP32 optimal CPU complexity balance (0-10)
        .enable_fec = false,                                                               // Disable Forward Error Correction
        .enable_dtx = false,                                                               // Disable Discontinuous Transmission
        .enable_vbr = true,                                                                // Enable Variable Bitrate mode
    };
// Wrap it in the generic open configuration structure
    esp_audio_enc_config_t cfg = {
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &m_opus_cfg,
        .cfg_sz = sizeof(m_opus_cfg)
    };
    // Open the Opus encoder instance passing configuration, structure size, and handle reference pointer
    esp_audio_err_t err = esp_audio_enc_open(&cfg,  &m_handle);

    // Verify if initialization was successful and handle was populated
    if (err != ESP_AUDIO_ERR_OK || m_handle == nullptr) {
        ESP_LOGE(TAG, "esp_opus_enc_open failed with error code: 0x%x", err);
        m_handle = nullptr; // Ensure handle remains null on error
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Encoder ready (%d Hz, %d ch, %lu ms duration)", m_config.sample_rate, m_config.channels, m_config.frame_duration_ms);


    int required_input_bytes = 0;
int required_output_bytes = 0;

esp_audio_err_t size_err = esp_opus_enc_get_frame_size(
    m_handle,
    &required_input_bytes,
    &required_output_bytes
);

if (size_err != ESP_AUDIO_ERR_OK) {
    ESP_LOGE(TAG, "Failed to query Opus frame sizes: 0x%x", size_err);
    deinit();
    return ESP_FAIL;
}

ESP_LOGI(TAG, "Opus frame sizes: input=%d bytes, output=%d bytes",
         required_input_bytes, required_output_bytes);
    return ESP_OK;
}

// Closes and frees the encoder memory
void SimpleOpusEncoder::deinit() {
    // Only attempt cleanup if handle is valid
    if (m_handle != nullptr) {
        esp_opus_enc_close(m_handle); // Call Espressif close function
        m_handle = nullptr;            // Reset pointer handle to null
        ESP_LOGI(TAG, "Opus encoder successfully closed");
    }
}

// Main encoding function: processes PCM audio into a raw buffer
int SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size) {
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
    in_frame.buffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(pcm_samples.data()));
    
    // Set total length of input data in bytes (sample_count * 2)
    in_frame.len = static_cast<uint32_t>(pcm_samples.size() * sizeof(int16_t));

    // Prepare output frame structure to hold compressed packet data
    esp_audio_enc_out_frame_t out_frame = {};
    out_frame.buffer = out_buffer;            // Point to destination output memory
    out_frame.len = out_buffer_max_size;    // Pass max available capacity of destination buffer
    out_frame.encoded_bytes = 0;            // Initialize output size accumulator to zero
    out_frame.pts = 0;                      // Initialize presentation timestamp to zero

    // Perform actual Opus compression encoding
    ESP_LOGI(TAG, "Encoding check: pcm_samples.size() = %u, out_buffer_max_size = %u", 
             (unsigned int)pcm_samples.size(), (unsigned int)out_buffer_max_size);

    // CHANGED: Mathematically calculated expected total sample count based on sample rate, channels, and duration (e.g., 16000 * 2 channels * 0.020s = 640 samples)
    size_t expected_samples_per_channel = (m_config.sample_rate * m_config.frame_duration_ms) / 1000;
    size_t expected_total_samples = expected_samples_per_channel * m_config.channels;

    if (pcm_samples.size() != expected_total_samples) { 
        ESP_LOGE(TAG, "CRITICAL: PCM sample count mismatch! Expected %zu, got %u", expected_total_samples, (unsigned int)pcm_samples.size());
    }

    ESP_LOGI("++++++++++++", "before esp_opus_enc_process  ");
    esp_audio_err_t err = esp_audio_enc_process(m_handle, &in_frame, &out_frame);

   /// ESP_LOGI("++++++++++++", "success esp_opus_enc_process  ");
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
std::vector<uint8_t> SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size) {
    // Allocate temporary vector with maximum allocated capacity
    ESP_LOGI("++++++++++++", "before  std::vector<uint8_t> output_vec(max_output_size);");
    std::vector<uint8_t> output_vec(max_output_size); 
    // Call raw buffer encoding method passing vector buffer memory
    int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);
    //ESP_LOGI("++++++++++++", "success    int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);");
    
    // Check if encoding produced output
    if (bytes_written > 0) {
        // Shrink vector down to match exact encoded byte length
        output_vec.resize(bytes_written);
        return output_vec;
    }

    // Return empty vector on failure or missing samples
    return {};
}














// #include "Opus.hpp"
// #include "esp_log.h"

// static const char* TAG = "SimpleOpusEncoder";

// SimpleOpusEncoder::SimpleOpusEncoder(const Config& config)
//     : m_config(config), m_encoder(nullptr), m_expected_samples_per_channel(0) {}

// SimpleOpusEncoder::~SimpleOpusEncoder() {
//     deinit();
// }

// SimpleOpusEncoder::SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept
//     : m_config(other.m_config), 
//       m_encoder(other.m_encoder), 
//       m_expected_samples_per_channel(other.m_expected_samples_per_channel) {
//     other.m_encoder = nullptr;
//     other.m_expected_samples_per_channel = 0;
// }

// SimpleOpusEncoder& SimpleOpusEncoder::operator=(SimpleOpusEncoder&& other) noexcept {
//     if (this != &other) {
//         deinit();
//         m_config = other.m_config;
//         m_encoder = other.m_encoder;
//         m_expected_samples_per_channel = other.m_expected_samples_per_channel;
//         other.m_encoder = nullptr;
//         other.m_expected_samples_per_channel = 0;
//     }
//     return *this;
// }

// esp_err_t SimpleOpusEncoder::init() {
//     if (m_encoder != nullptr) {
//         ESP_LOGW(TAG, "Encoder handle already initialized");
//         return ESP_OK;
//     }

//     int err = 0;
//     // Create the upstream Opus encoder instance with fixed-point arithmetic optimization
//     m_encoder = opus_encoder_create(m_config.sample_rate, m_config.channels, OPUS_APPLICATION_VOIP, &err);
    
//     if (err != OPUS_OK || m_encoder == nullptr) {
//         ESP_LOGE(TAG, "opus_encoder_create failed with error: %s", opus_strerror(err));
//         m_encoder = nullptr;
//         return ESP_FAIL;
//     }

//     // Configure encoder parameters via control controls (ctl)
//     opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(m_config.bitrate));
//     opus_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(m_config.complexity));
//     opus_encoder_ctl(m_encoder, OPUS_SET_VBR(1));          // Enable Variable Bitrate
//     opus_encoder_ctl(m_encoder, OPUS_SET_VBR_CONSTRAINT(1)); // Constrained VBR for steady streaming

//     // Calculate expected samples per channel based on frame duration
//     m_expected_samples_per_channel = (m_config.sample_rate * m_config.frame_duration_ms) / 1000;

//     ESP_LOGI(TAG, "Upstream Opus Encoder initialized successfully (%d Hz, %d ch, %lu ms duration, %zu samples/ch)",
//              m_config.sample_rate, m_config.channels, m_config.frame_duration_ms, m_expected_samples_per_channel);
    
//     return ESP_OK;
// }

// void SimpleOpusEncoder::deinit() {
//     if (m_encoder != nullptr) {
//         opus_encoder_destroy(m_encoder);
//         m_encoder = nullptr;
//         m_expected_samples_per_channel = 0;
//         ESP_LOGI(TAG, "Upstream Opus Encoder destroyed successfully");
//     }
// }

// int SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size) {
//     if (m_encoder == nullptr) {
//         ESP_LOGE(TAG, "Encoder is uninitialized. Call init() first.");
//         return -1;
//     }

//     if (pcm_samples.empty() || out_buffer == nullptr || out_buffer_max_size == 0) {
//         return 0;
//     }

//     size_t expected_total_samples = m_expected_samples_per_channel * m_config.channels;
//         ESP_LOGI(TAG, "  Expected %zu, got %u", 
//                  expected_total_samples, (unsigned int)pcm_samples.size());

//     if (pcm_samples.size() != expected_total_samples) {
//         ESP_LOGE(TAG, "PCM sample count mismatch! Expected %zu, got %u", 
//                  expected_total_samples, (unsigned int)pcm_samples.size());

//                  return -1;
//     }
// ESP_LOGI(TAG, "try opus_encode");
//     // Invoke direct upstream libopus encoding routine

//     int encoded_bytes = opus_encode(
//         m_encoder,
//         pcm_samples.data(),
//         static_cast<int>(m_expected_samples_per_channel),
//         out_buffer,
//         static_cast<int>(out_buffer_max_size)
//     );

// ESP_LOGI(TAG, "done opus_encode");
//     if (encoded_bytes < 0) {
//         ESP_LOGE(TAG, "opus_encode failed: %s", opus_strerror(encoded_bytes));
//         return -1;
//     }

//     return encoded_bytes;
// }

// std::vector<uint8_t> SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size) {
//     std::vector<uint8_t> output_vec(max_output_size);
//     int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);
    
//     if (bytes_written > 0) {
//         output_vec.resize(bytes_written);
//         return output_vec;
//     }

//     return {};
// }




// #include "Opus.hpp" // Include corresponding class declaration header
// #include "esp_log.h"             // Include ESP-IDF logging library facilities

// static const char* TAG = "SimpleOpusDecoder"; // Define log tag for terminal monitoring filters

// // Constructor initialization list copying input configuration and resetting handle pointer
// SimpleOpusDecoder::SimpleOpusDecoder(const Config& config)
//     : m_config(config), m_dec_handle(nullptr) {}

// // Destructor invokes deinit() to guarantee safe release of system resources
// SimpleOpusDecoder::~SimpleOpusDecoder() {
//     deinit();
// }

// // Move constructor transferring ownership of the decoder handle and zeroing source trackers
// SimpleOpusDecoder::SimpleOpusDecoder(SimpleOpusDecoder&& other) noexcept
//     : m_config(other.m_config), m_dec_handle(other.m_dec_handle) {
//     other.m_dec_handle = nullptr; // Nullify source handle pointer to avoid double-close bugs
// }

// // Move assignment operator guarding against self-assignment and transferring state handles
// SimpleOpusDecoder& SimpleOpusDecoder::operator=(SimpleOpusDecoder&& other) noexcept {
//     if (this != &other) { // Verify source object is unique from current instance
//         deinit(); // Clean up current decoder resources prior to assignment
//         m_config = other.m_config; // Copy configuration structure payload
//         m_dec_handle = other.m_dec_handle; // Take ownership of opaque handle pointer
//         other.m_dec_handle = nullptr; // Invalidate source handle reference
//     }
//     return *this; // Return reference to updated object instance
// }

// // Initialization method wrapping esp_opus_dec_open with user configuration parameters
// esp_err_t SimpleOpusDecoder::init() {
//     if (m_dec_handle != nullptr) { // Check if decoder handle is already running
//         ESP_LOGW(TAG, "Espressif Opus decoder handle already initialized"); // Emit warning notice
//         return ESP_OK; // Return success status safely
//     }

//     // Populate Espressif component configuration structure using class settings
//     esp_opus_dec_cfg_t cfg = {
//         .sample_rate = m_config.sample_rate,
//         .channel = m_config.channel,
//         .frame_duration = m_config.frame_duration,
//         .self_delimited = m_config.self_delimited
//     };

//     // Invoke Espressif's component decoder open routine
//     esp_audio_err_t err = esp_opus_dec_open(&cfg, sizeof(cfg), &m_dec_handle);
    
//     if (err != ESP_AUDIO_ERR_OK || m_dec_handle == nullptr) { // Validate opening success
//         ESP_LOGE(TAG, "esp_opus_dec_open failed with error code: %d", err); // Log failure code
//         m_dec_handle = nullptr; // Ensure pointer remains null on error
//         return ESP_FAIL; // Return failure status
//     }

//     ESP_LOGI(TAG, "Espressif Opus Decoder initialized successfully (Rate: %lu Hz, Ch: %u)",
//              m_config.sample_rate, m_config.channel);

//     return ESP_OK; // Return success status code
// }

// // Deinitialization method cleanly closing the decoder handle via Espressif framework hooks
// void SimpleOpusDecoder::deinit() {
//     if (m_dec_handle != nullptr) { // Verify an active handle exists
//         esp_opus_dec_close(m_dec_handle); // Call native framework closing function
//         m_dec_handle = nullptr; // Reset handle pointer to null
//         ESP_LOGI(TAG, "Espressif Opus Decoder closed successfully"); // Log cleanup notice
//     }
// }

// // Decoding method feeding compressed byte packets into Espressif's pipeline and extracting PCM data
// std::vector<int16_t> SimpleOpusDecoder::decode(const uint8_t* opus_data, uint32_t opus_data_size) {
//     if (m_dec_handle == nullptr) { // Guard against uninitialized executions
//         ESP_LOGE(TAG, "Decoder is uninitialized. Call init() first."); // Log initialization error
//         return {}; // Return empty container vector
//     }

//     if (opus_data == nullptr || opus_data_size == 0) { // Validate input payload bounds
//         return {}; // Return empty vector
//     }

//     // Wrap input compressed stream data inside Espressif's raw input structure container
//     esp_audio_dec_in_raw_t raw_input = {
//         .buffer = const_cast<uint8_t*>(opus_data),
//         .len = opus_data_size
//     };

//     // Allocate a safe backing output PCM buffer array (960 samples max per frame layout)
//     std::vector<int16_t> pcm_output_buffer(960);

//     // Wrap output buffer inside Espressif's output frame structure container
//     esp_audio_dec_out_frame_t out_frame = {
//         .buffer = reinterpret_cast<uint8_t*>(pcm_output_buffer.data()),
//         .len = static_cast<uint32_t>(pcm_output_buffer.size() * sizeof(int16_t))
//     };

//     // Container structure to hold decoder stream update metadata
//     esp_audio_dec_info_t dec_info = {};

//     // Invoke Espressif's core stream decoding routine
//     esp_audio_err_t err = esp_opus_dec_decode(m_dec_handle, &raw_input, &out_frame, &dec_info);

//     if (err != ESP_AUDIO_ERR_OK) { // Verify decoding execution success
//         ESP_LOGE(TAG, "esp_opus_dec_decode failed with error code: %d", err); // Log error code
//         return {}; // Return empty vector on failure
//     }

//     // Calculate exact number of decoded 16-bit PCM samples returned from output length
//     size_t decoded_sample_count = out_frame.len / sizeof(int16_t);
//     pcm_output_buffer.resize(decoded_sample_count); // Resize vector to fit exact output sample footprint

//     return pcm_output_buffer; // Return container vector filled with decoded PCM samples
// }













// #include "Opus.hpp" // Include corresponding class declaration header
// #include "esp_log.h"             // Include ESP-IDF logging library facilities

// static const char* TAG = "SimpleOpusEncoder"; // Define log tag for terminal monitoring filters

// // Constructor initialization list copying input configuration and resetting handle pointer
// SimpleOpusEncoder::SimpleOpusEncoder(const Config& config)
//     : m_config(config), m_enc_handle(nullptr) {}

// // Destructor invokes deinit() to guarantee safe release of system resources
// SimpleOpusEncoder::~SimpleOpusEncoder() {
//     deinit();
// }

// // Move constructor transferring ownership of the encoder handle and zeroing source trackers
// SimpleOpusEncoder::SimpleOpusEncoder(SimpleOpusEncoder&& other) noexcept
//     : m_config(other.m_config), m_enc_handle(other.m_enc_handle) {
//     other.m_enc_handle = nullptr; // Nullify source handle pointer to avoid double-close bugs
// }

// // Move assignment operator guarding against self-assignment and transferring state handles
// SimpleOpusEncoder& SimpleOpusEncoder::operator=(SimpleOpusEncoder&& other) noexcept {
//     if (this != &other) { // Verify source object is unique from current instance
//         deinit(); // Clean up current encoder resources prior to assignment
//         m_config = other.m_config; // Copy configuration structure payload
//         m_enc_handle = other.m_enc_handle; // Take ownership of opaque handle pointer
//         other.m_enc_handle = nullptr; // Invalidate source handle reference
//     }
//     return *this; // Return reference to updated object instance
// }

// // Initialization method wrapping esp_opus_enc_open with user configuration parameters
// esp_err_t SimpleOpusEncoder::init() {
//     if (m_enc_handle != nullptr) { // Check if encoder handle is already running
//         ESP_LOGW(TAG, "Espressif Opus encoder handle already initialized"); // Emit warning notice
//         return ESP_OK; // Return success status safely
//     }

//     // Populate Espressif component configuration structure using class settings
//     esp_opus_enc_config_t cfg = {
//         .sample_rate = m_config.sample_rate,
//         .channel = m_config.channel,
//         .bits_per_sample = m_config.bits_per_sample,
//         .bitrate = m_config.bitrate,
//         .frame_duration = m_config.frame_duration,
//         .complexity = m_config.complexity
//     };

//     // Invoke Espressif's component encoder open routine
//     esp_audio_err_t err = esp_opus_enc_open(&cfg, sizeof(cfg), &m_enc_handle);
    
//     if (err != ESP_AUDIO_ERR_OK || m_enc_handle == nullptr) { // Validate opening success
//         ESP_LOGE(TAG, "esp_opus_enc_open failed with error code: %d", err); // Log failure code
//         m_enc_handle = nullptr; // Ensure pointer remains null on error
//         return ESP_FAIL; // Return failure status
//     }

//     ESP_LOGI(TAG, "Espressif Opus Encoder initialized successfully (Rate: %lu Hz, Ch: %u, Bitrate: %d bps)",
//              m_config.sample_rate, m_config.channel, m_config.bitrate);

//     return ESP_OK; // Return success status code
// }

// // Deinitialization method cleanly closing the encoder handle via Espressif framework hooks
// void SimpleOpusEncoder::deinit() {
//     if (m_enc_handle != nullptr) { // Verify an active handle exists
//         esp_opus_enc_close(m_enc_handle); // Call native framework closing function
//         m_enc_handle = nullptr; // Reset handle pointer to null
//         ESP_LOGI(TAG, "Espressif Opus Encoder closed successfully"); // Log cleanup notice
//     }
// }

// // Core encoding method wrapping PCM frames into Espressif's raw input structures and invoking compression
// int SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size) {
//     if (m_enc_handle == nullptr) { // Guard against uninitialized executions
//         ESP_LOGE(TAG, "Encoder is uninitialized. Call init() first."); // Log initialization error
//         return -1; // Return error indicator integer
//     }

//     if (pcm_samples.empty() || out_buffer == nullptr || out_buffer_max_size == 0) { // Validate input buffer bounds
//         return 0; // Return zero bytes processed safely
//     }

//     // Wrap input raw PCM sample data inside Espressif's input structure container
//     esp_audio_dec_in_raw_t raw_input = {
//         .buffer = (uint8_t*)pcm_samples.data(),
//         .len = static_cast<uint32_t>(pcm_samples.size() * sizeof(int16_t))
//     };

//     // Wrap destination buffer inside Espressif's output frame structure container
//     esp_audio_dec_out_frame_t out_frame = {
//         .buffer = out_buffer,
//         .len = out_buffer_max_size
//     };

//     esp_audio_dec_info_t enc_info = {};

//     // Invoke Espressif's core stream encoding routine
//     esp_audio_err_t err = esp_opus_enc_encode(m_enc_handle, &raw_input, &out_frame, &enc_info);

//     if (err != ESP_AUDIO_ERR_OK) { // Verify encoding execution success
//         ESP_LOGE(TAG, "esp_opus_enc_encode failed with error code: %d", err); // Log error code
//         return -1; // Return error indicator integer
//     }

//     return static_cast<int>(out_frame.len); // Return exact number of compressed bytes written
// }

// // Convenience wrapper vector method allocating a container and redirecting to the primary encode function
// std::vector<uint8_t> SimpleOpusEncoder::encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size) {
//     std::vector<uint8_t> output_vec(max_output_size); // Instantiate local vector buffer allocation
    
//     // Execute encoding routine targeting the internal raw data pointer of the temporary vector
//     int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);
    
//     if (bytes_written > 0) { // Check if compression wrote valid output bytes
//         output_vec.resize(bytes_written); // Resize vector array to fit exact compressed frame size
//         return output_vec; // Return populated vector container
//     }

//     return {}; // Return empty container vector on failure
// }