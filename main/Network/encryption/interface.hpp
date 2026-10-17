#pragma once







#include "psa/crypto.h"
#include "esp_random.h"
#include "esp_log.h"
#include <cstring>

#include <vector>
#include <optional>

#include <vector>
#include <optional>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"


namespace encrytion {
static const char *TAG = "AES_GCM";

constexpr size_t AES_KEY_BYTES = 32;   // 256-bit AES Key
constexpr size_t GCM_NONCE_BYTES = 12; // Standard 12-byte Nonce (IV)
constexpr size_t GCM_TAG_BYTES = 16;   // Standard 16-byte Auth Tag



// Helper to initialize the PSA Crypto subsystem once
inline bool init_crypto() {
    static bool initialized = false;
    if (!initialized) {
        psa_status_t status = psa_crypto_init();
        if (status != PSA_SUCCESS) {
            ESP_LOGE(TAG, "Failed to initialize PSA crypto engine: %d", status);
            return false;
        }
        initialized = true;
    }
    return true;
}



/**
 * @brief Encrypts plaintext using mbedTLS AES-256-GCM.
 * 
 * @param key          32-byte shared key
 * @param plaintext    Raw buffer to encrypt (e.g. KCP output payload)
 * @param len          Length of plaintext
 * @param out_buffer   Output buffer for encrypted packet
 * @param out_len      [in/out] Provided buffer capacity / output actual length
 * @return true on success, false on failure
 */
bool aes_gcm_encrypt(const uint8_t key[AES_KEY_BYTES],
                     const uint8_t* plaintext, size_t len,
                     uint8_t* out_buffer, size_t* out_len);



std::optional<std::vector<uint8_t>> aes_gcm_encrypt( const std::vector<uint8_t>& plaintext);





/**
 * @brief Decrypts an incoming AES-256-GCM packet.
 * 
 * @param key          32-byte shared key
 * @param packet       Received raw UDP buffer [12B Nonce | 16B Tag | Ciphertext]
 * @param packet_len   Length of received packet
 * @param plaintext_out Output buffer for decrypted payload
 * @param plaintext_len [in/out] Provided buffer size / actual decrypted size
 * @return true on success, false if tag verification fails
 */
bool aes_gcm_decrypt(const uint8_t key[AES_KEY_BYTES],
                     const uint8_t* packet, size_t packet_len,
                     uint8_t* plaintext_out, size_t* plaintext_len);








std::optional<std::vector<uint8_t>> aes_gcm_decrypt(const std::vector<uint8_t>& packet);






// Function that loads the AES key from NVS and returns it as a std::vector inside std::optional
std::optional<std::vector<uint8_t>> load_aes_key_from_nvs() ;







} ;


























