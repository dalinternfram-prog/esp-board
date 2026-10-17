#include "interface.hpp"
#include <cstring>

static const char *TAG = "PSA_AES_GCM";

namespace encrytion {


 bool aes_gcm_decrypt(const uint8_t key[AES_KEY_BYTES],                     // Function signature taking key, packet, packet length, and output pointers
                     const uint8_t* packet, size_t packet_len,             
                     uint8_t* plaintext_out, size_t* plaintext_len)        
{                                                                          
    if (!init_crypto()) return false;                                      // Ensure the underlying crypto subsystem is initialized

    // Must contain at least Nonce (12B) + Tag (16B)                       
    if (packet_len < GCM_NONCE_BYTES + GCM_TAG_BYTES) {                    // Verify packet is large enough to contain at least the nonce and tag
        ESP_LOGE(TAG, "Packet too short");                                 // Log error if packet is too small
        return false;                                                      // Return false to indicate failure
    }                                                                      

    size_t ciphertext_len = packet_len - GCM_NONCE_BYTES - GCM_TAG_BYTES;  // Calculate the actual length of the ciphertext payload
    if (*plaintext_len < ciphertext_len) {                                 // Check if the provided output buffer is large enough for the decrypted data
        ESP_LOGE(TAG, "Output buffer too small");                          // Log error if buffer capacity is insufficient
        return false;                                                      // Return false to indicate failure
    }                                                                      

    // Packet Layout: [12B Nonce | 16B Tag | Ciphertext]                    
    const uint8_t* nonce = packet;                                         // Set pointer to the nonce at the beginning of the packet
    const uint8_t* tag = packet + GCM_NONCE_BYTES;                         // Set pointer to the authentication tag immediately following the nonce
    const uint8_t* ciphertext = packet + GCM_NONCE_BYTES + GCM_TAG_BYTES;  // Set pointer to the ciphertext starting after the tag

    // 1. Configure PSA key attributes                                      
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;             // Initialize PSA key attributes structure
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);           // Configure key attributes to specifically allow decryption operations
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);                       // Set algorithm to Galois/Counter Mode (GCM)
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);                       // Set key type to AES
    psa_set_key_bits(&attributes, AES_KEY_BYTES * 8);                      // Set key size in bits

    // 2. Import secret key into PSA context                                
    psa_key_id_t key_id;                                                   // Declare variable for PSA key identifier
    psa_status_t status = psa_import_key(&attributes, key, AES_KEY_BYTES, &key_id); // Import raw key into PSA subsystem
    if (status != PSA_SUCCESS) {                                           // Check if key import succeeded
        ESP_LOGE(TAG, "Failed to import key: %d", status);                 // Log error if import failed
        return false;                                                      // Return false to indicate failure
    }                                                                      

    // 3. PSA expects Tag concatenated immediately after Ciphertext         
    uint8_t encrypted_payload[ciphertext_len + GCM_TAG_BYTES];             // Allocate temporary buffer for ciphertext + tag layout required by PSA
    std::memcpy(encrypted_payload, ciphertext, ciphertext_len);            // Copy ciphertext into the temporary payload buffer
    std::memcpy(encrypted_payload + ciphertext_len, tag, GCM_TAG_BYTES);   // Copy the authentication tag directly after the ciphertext

    // 4. Perform Authenticated Decryption                                  
    size_t decrypted_size = 0;                                             // Initialize variable to track actual decrypted plaintext length
    status = psa_aead_decrypt(                                             // Perform AEAD decryption and authentication tag verification
        key_id,                                                            // Pass imported key ID
        PSA_ALG_GCM,                                                       // Specify GCM algorithm
        nonce, GCM_NONCE_BYTES,                                            // Provide nonce and its size
        nullptr, 0, // No AAD                                              // Additional Authenticated Data (none used here)
        encrypted_payload, sizeof(encrypted_payload),                      // Provide combined ciphertext + tag payload
        plaintext_out, *plaintext_len,                                     // Destination buffer and its allocated capacity
        &decrypted_size                                                    // Pointer to receive actual decrypted output length
    );                                                                     

    // Clean up key from RAM                                                
    psa_destroy_key(key_id);                                               // Securely destroy the imported key from PSA memory

    if (status != PSA_SUCCESS) {                                           // Check if decryption or tag verification failed
        ESP_LOGE(TAG, "Decryption or auth tag verification failed! Error: %d", status); // Log verification/decryption failure
        return false;                                                      // Return false to indicate failure
    }                                                                      

    *plaintext_len = decrypted_size;                                       // Update output length variable to actual decrypted size
    return true;                                                           // Return true to indicate successful decryption
}

// Wrapper function for decryption using std::vector and std::optional
std::optional<std::vector<uint8_t>> aes_gcm_decrypt( 
    const std::vector<uint8_t>& packet)
{

        auto __Key = load_aes_key_from_nvs();
    if(!__Key.has_value())   return std::nullopt;

   auto key = __Key.value();

   
    // Validate key size
    if (key.size() != AES_KEY_BYTES) {
        ESP_LOGE(TAG, "Invalid key size: expected %d, got %u", AES_KEY_BYTES, key.size());
        return std::nullopt;
    }

    // Validate minimum packet length (Nonce + Tag)
    if (packet.size() < GCM_NONCE_BYTES + GCM_TAG_BYTES) {
        ESP_LOGE(TAG, "Packet too short");
        return std::nullopt;
    }

    // Max possible plaintext length
    size_t plaintext_len = packet.size() - GCM_NONCE_BYTES - GCM_TAG_BYTES;
    std::vector<uint8_t> plaintext_out(plaintext_len);

    // Call your original decryption function
    bool success = aes_gcm_decrypt(
        key.data(),
        packet.data(),
        packet.size(),
        plaintext_out.data(),
        &plaintext_len
    );

    // Return std::nullopt if decryption or auth check failed
    if (!success) {
        return std::nullopt;
    }

    // Adjust vector size to match the actual decrypted length
    plaintext_out.resize(plaintext_len);

    return plaintext_out;
}
} // namespace encrytion