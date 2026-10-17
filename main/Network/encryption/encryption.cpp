#include "interface.hpp"
#include <cstring>

static const char *TAG = "PSA_AES_GCM";

namespace encrytion {

 

 bool aes_gcm_encrypt(const uint8_t key[AES_KEY_BYTES],                     // Function signature taking the key, plaintext, its length, and output buffer pointers
                     const uint8_t* plaintext, size_t len,                 
                     uint8_t* out_buffer, size_t* out_len)                 
{                                                                          
    if (!init_crypto()) return false;                                      // Ensure the underlying crypto subsystem is initialized before proceeding

    if (*out_len < len + GCM_NONCE_BYTES + GCM_TAG_BYTES) {                // Check if the provided output buffer is large enough for Nonce + Tag + Ciphertext
        ESP_LOGE(TAG, "Buffer capacity too small");                        // Log an error if the buffer capacity is insufficient
        return false;                                                      // Return false to indicate failure
    }                                                                      

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;             // Initialize PSA key attributes structure with default settings
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);           // Configure the key attributes to specifically allow encryption operations
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);                       // Set the cryptographic algorithm to Galois/Counter Mode (GCM)
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);                       // Specify that the key type is AES
    psa_set_key_bits(&attributes, AES_KEY_BYTES * 8);                      // Define the key size in bits (converting bytes to bits)

    psa_key_id_t key_id;                                                   // Declare a variable to store the PSA internal key identifier
    psa_status_t status = psa_import_key(&attributes, key, AES_KEY_BYTES, &key_id); // Import the raw key into the PSA subsystem and retrieve its ID
    if (status != PSA_SUCCESS) {                                           // Check if the key import operation was successful
        ESP_LOGE(TAG, "Failed to import key: %d", status);                 // Log the specific error code if import failed
        return false;                                                      // Return false to indicate failure
    }                                                                      

    uint8_t nonce[GCM_NONCE_BYTES];                                        // Declare a local stack buffer for the random initialization vector (nonce)
    esp_fill_random(nonce, GCM_NONCE_BYTES);                               // Fill the nonce buffer with cryptographically secure random bytes

    uint8_t encrypted_payload[len + GCM_TAG_BYTES];                        // Declare a temporary buffer sized to hold both the ciphertext and the authentication tag
    size_t encrypted_output_len = 0;                                       // Initialize a variable to track the actual number of bytes written during encryption

    status = psa_aead_encrypt(                                             // Perform the AEAD encryption using the PSA cryptographic API
        key_id,                                                            // Pass the imported key identifier
        PSA_ALG_GCM,                                                       // Specify the AEAD algorithm (GCM)
        nonce, GCM_NONCE_BYTES,                                            // Provide the nonce pointer and its length
        nullptr, 0,                                                        // Additional Authenticated Data (AAD) pointer and length (none used here)
        plaintext, len,                                                    // Provide the source plaintext pointer and its length
        encrypted_payload, sizeof(encrypted_payload),                      // Provide the destination buffer and its total allocated size
        &encrypted_output_len                                              // Pointer to receive the actual written output length
    );                                                                     

    psa_destroy_key(key_id);                                               // Securely destroy the imported key from the PSA subsystem to clean up memory

    if (status != PSA_SUCCESS) {                                           // Check if the encryption process succeeded
        ESP_LOGE(TAG, "Encryption failed: %d", status);                    // Log the error status code if encryption failed
        return false;                                                      // Return false to indicate failure
    }                                                                      

    // Format output buffer: [12B Nonce | 16B Tag | Ciphertext]
    std::memcpy(out_buffer, nonce, GCM_NONCE_BYTES);                       // Copy the random nonce into the very beginning of the final output buffer
    
    // In PSA, the authentication tag is appended at the very end of the encrypted payload output
    std::memcpy(out_buffer + GCM_NONCE_BYTES, encrypted_payload + len, GCM_TAG_BYTES); // Copy the authentication tag right after the nonce
    std::memcpy(out_buffer + GCM_NONCE_BYTES + GCM_TAG_BYTES, encrypted_payload, len); // Copy the actual ciphertext immediately following the tag

    *out_len = len + GCM_NONCE_BYTES + GCM_TAG_BYTES;                      // Update the output length variable to reflect the total final payload size
    return true;                                                           // Return true to indicate successful completion
}



// Wrapper function that calls the original implementation using std::vector
std::optional<std::vector<uint8_t>> aes_gcm_encrypt(
    const std::vector<uint8_t>& plaintext)
{

// load key;
    auto __Key = load_aes_key_from_nvs();
    if(!__Key.has_value())   return std::nullopt;

   auto key = __Key.value();



    // Validate that the key matches the expected byte length
    if (key.size() != AES_KEY_BYTES) {
        ESP_LOGE(TAG, "Invalid key size: expected %d, got %u", AES_KEY_BYTES, key.size());
        return std::nullopt;
    }

    // Determine the required buffer size (Nonce + Tag + Plaintext length)
    size_t out_len = plaintext.size() + GCM_NONCE_BYTES + GCM_TAG_BYTES;
    std::vector<uint8_t> out_buffer(out_len);

    // Call your original function
    bool success = aes_gcm_encrypt(
        key.data(),                    // Raw key pointer
        plaintext.data(),              // Raw plaintext pointer
        plaintext.size(),              // Plaintext length
        out_buffer.data(),             // Output buffer pointer
        &out_len                       // Input buffer capacity, updated to actual length on success
    );

    // Return std::nullopt if encryption failed
    if (!success) {
        return std::nullopt;
    }

    // Adjust vector size to match the final written output length
    out_buffer.resize(out_len);

    return out_buffer;
}









} // namespace encrytion