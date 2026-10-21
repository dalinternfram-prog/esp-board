



// #include "Lc3.hpp" // Include corresponding class declaration header
// #include "esp_log.h"            // Include ESP-IDF logging library facilities

// static const char* TAG = "SimpleLC3Encoder"; // Define string log tag for filtering logs in terminal monitor

// // Constructor initialization list copying input configuration and resetting trackers
// SimpleLC3Encoder::SimpleLC3Encoder(const Config& config)
//     : m_config(config), m_encoder(nullptr), m_expected_samples_per_channel(0) {}

// // Destructor invokes deinit() to guarantee safe teardown of system resources
// SimpleLC3Encoder::~SimpleLC3Encoder() {
//     deinit();
// }

// // Move constructor transferring ownership of internal members and zeroing out source objects
// SimpleLC3Encoder::SimpleLC3Encoder(SimpleLC3Encoder&& other) noexcept
//     : m_config(other.m_config),
//       m_encoder_mem(std::move(other.m_encoder_mem)),
//       m_encoder(other.m_encoder),
//       m_expected_samples_per_channel(other.m_expected_samples_per_channel),
//       m_target_bytes(other.m_target_bytes) {
//     other.m_encoder = nullptr; // Nullify source pointer to prevent double-free bugs
//     other.m_expected_samples_per_channel = 0; // Reset source sample size tracker
//     other.m_target_bytes = 0; // Reset source target byte allocation size
// }

// // Move assignment operator safeguarding against self-assignment and transferring state vectors
// SimpleLC3Encoder& SimpleLC3Encoder::operator=(SimpleLC3Encoder&& other) noexcept {
//     if (this != &other) { // Verify source object is distinct from current instance
//         deinit(); // Clean up current resources prior to accepting new ownership
//         m_config = other.m_config; // Copy configuration payload
//         m_encoder_mem = std::move(other.m_encoder_mem); // Move internal memory buffer vector
//         m_encoder = other.m_encoder; // Copy opaque encoder handle pointer
//         m_expected_samples_per_channel = other.m_expected_samples_per_channel; // Copy sample counts
//         m_target_bytes = other.m_target_bytes; // Copy target frame byte length
//         other.m_encoder = nullptr; // Invalidate source pointer handle
//         other.m_expected_samples_per_channel = 0; // Clear source metadata
//         other.m_target_bytes = 0; // Clear source byte targets
//     }
//     return *this; // Return reference to updated object instance
// }

// // Initialization method verifying parameters, calculating buffer requirements, and binding state pointers
// esp_err_t SimpleLC3Encoder::init() {
//     if (m_encoder != nullptr) { // Check if encoder is already running
//         ESP_LOGW(TAG, "LC3 encoder handle already initialized"); // Emit warning log message
//         return ESP_OK; // Return success status safely
//     }

//     // Compute expected sample count per channel: (SampleRate * DurationInSeconds)
//     m_expected_samples_per_channel = (m_config.sample_rate * (m_config.frame_duration_us / 1000)) / 1000;

//     // Compute exact target compressed bytes per frame from target bitrate: (Bitrate * DurationSeconds) / 8 bits
//     m_target_bytes = (m_config.bitrate * (int)(m_config.frame_duration_us / 1000)) / (8 * 1000);

//     // Query required memory size in bytes from liblc3 for given frame settings
//     int enc_size = lc3_encoder_size(m_config.frame_duration_us, m_config.sample_rate);
//     if (enc_size <= 0) { // Check if returned size is invalid or unsupported
//         ESP_LOGE(TAG, "Invalid encoder parameters supplied to lc3_encoder_size()"); // Log fatal error
//         return ESP_FAIL; // Return failure error code
//     }

//     // Resize internal memory vector container to fulfill liblc3 scratch-pad byte storage requirement
//     m_encoder_mem.resize(enc_size);

//     // Setup and bind encoder state pointer inside the pre-allocated vector memory block
//     m_encoder = lc3_setup_encoder(m_config.frame_duration_us, m_config.sample_rate, m_config.epmr, m_encoder_mem.data());
//     if (m_encoder == nullptr) { // Validate pointer creation success
//         ESP_LOGE(TAG, "lc3_setup_encoder failed to initialize context pointer"); // Log failure detail
//         return ESP_FAIL; // Return failure code
//     }

//     // Log successful initialization summary including configuration metrics
//     ESP_LOGI(TAG, "Google liblc3 Encoder initialized successfully (%d Hz, %d ch, %lu us, %zu samples/ch, %d target bytes)",
//              m_config.sample_rate, m_config.channels, m_config.frame_duration_us, 
//              m_expected_samples_per_channel, m_target_bytes);

//     return ESP_OK; // Return success status code
// }

// // Deinitialization method safely wiping references and resetting operational variables
// void SimpleLC3Encoder::deinit() {
//     if (m_encoder != nullptr) { // Verify if an active encoder handle exists
//         m_encoder = nullptr; // Clear out opaque reference pointer
//         m_encoder_mem.clear(); // Clear internal memory vector array
//         m_encoder_mem.shrink_to_fit(); // Release allocated RAM heap back to system
//         m_expected_samples_per_channel = 0; // Reset tracked sample sizing
//         m_target_bytes = 0; // Reset byte limits
//         ESP_LOGI(TAG, "Google liblc3 Encoder destroyed successfully"); // Log cleanup success notice
//     }
// }

// // Zero-copy core encoding method validating buffer constraints and invoking liblc3 compression logic
// int SimpleLC3Encoder::encode(const std::vector<int16_t>& pcm_samples, uint8_t* out_buffer, uint32_t out_buffer_max_size) {
//     if (m_encoder == nullptr) { // Guard against uninitialized function calls
//         ESP_LOGE(TAG, "Encoder is uninitialized. Call init() first."); // Log initialization error
//         return -1; // Return error indicator integer
//     }

//     if (pcm_samples.empty() || out_buffer == nullptr || out_buffer_max_size == 0) { // Validate parameters
//         return 0; // Return zero bytes processed safely
//     }

//     // Compute expected total sample count across all configured audio channels
//     size_t expected_total_samples = m_expected_samples_per_channel * m_config.channels;

//     if (pcm_samples.size() != expected_total_samples) { // Ensure input vector size matches frame layout
//         ESP_LOGE(TAG, "PCM sample count mismatch! Expected %zu, got %u", 
//                    expected_total_samples, (unsigned int)pcm_samples.size()); // Log exact discrepancy
//         return -1; // Return error code
//     }

//     if ((uint32_t)m_target_bytes > out_buffer_max_size) { // Verify output buffer capacity is large enough
//         ESP_LOGE(TAG, "Output buffer too small! Required: %d bytes, Available: %lu bytes", 
//                    m_target_bytes, (unsigned long)out_buffer_max_size); // Log buffer boundary issue
//         return -1; // Return error indicator
//     }

//     // Invoke upstream Google liblc3 routine to compress 16-bit signed PCM frames into raw byte streams
//     int err = lc3_encode(m_encoder, LC3_PCM_FORMAT_S16, pcm_samples.data(), m_config.channels, m_target_bytes, out_buffer);
    
//     if (err != 0) { // Check return error code (0 means successful encoding execution)
//         ESP_LOGE(TAG, "lc3_encode execution failed with error code: %d", err); // Log execution failure
//         return -1; // Return error indicator
//     }

//     return m_target_bytes; // Return exact number of compressed bytes written into output buffer
// }

// // Convenience wrapper vector method allocating a container and redirecting to the primary encode function
// std::vector<uint8_t> SimpleLC3Encoder::encode(const std::vector<int16_t>& pcm_samples, uint32_t max_output_size) {
//     std::vector<uint8_t> output_vec(max_output_size); // Instantiate local vector buffer allocation
    
//     // Execute encoding routine targeting the internal raw data pointer of the temporary vector
//     int bytes_written = encode(pcm_samples, output_vec.data(), max_output_size);
    
//     if (bytes_written > 0) { // Check if compression wrote valid output bytes
//         output_vec.resize(bytes_written); // Resize vector array to fit exact compressed frame size
//         return output_vec; // Return populated vector container
//     }

//     return {}; // Return empty container vector on failure
// }