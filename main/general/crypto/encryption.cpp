#include "crypto.hpp"

#include <cstring>

namespace encryption {

bool aes_gcm_encrypt(const uint8_t key[AES_KEY_BYTES],
                     const uint8_t* plaintext, size_t len,
                     uint8_t* out_buffer, size_t* out_len,
                     CryptoBackend& backend)
{
    if (!backend.init()) {
        return false;
    }

    if (*out_len < len + GCM_NONCE_BYTES + GCM_TAG_BYTES) {
        LOG_E(TAG, "Buffer capacity too small");
        return false;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, AES_KEY_BYTES * 8);

    psa_key_id_t key_id;
    psa_status_t status = psa_import_key(&attributes, key, AES_KEY_BYTES, &key_id);
    if (status != PSA_SUCCESS) {
        LOG_E(TAG, "Failed to import key: %d", static_cast<int>(status));
        return false;
    }

    uint8_t nonce[GCM_NONCE_BYTES];
    backend.fillRandom(nonce, GCM_NONCE_BYTES);

    std::vector<uint8_t> encrypted_payload(len + GCM_TAG_BYTES);
    size_t encrypted_output_len = 0;

    status = psa_aead_encrypt(
        key_id,
        PSA_ALG_GCM,
        nonce, GCM_NONCE_BYTES,
        nullptr, 0,  // no AAD
        plaintext, len,
        encrypted_payload.data(), encrypted_payload.size(),
        &encrypted_output_len);

    psa_destroy_key(key_id);

    if (status != PSA_SUCCESS) {
        LOG_E(TAG, "Encryption failed: %d", static_cast<int>(status));
        return false;
    }

    // Output layout: [12B nonce | 16B tag | ciphertext]
    std::memcpy(out_buffer, nonce, GCM_NONCE_BYTES);
    std::memcpy(out_buffer + GCM_NONCE_BYTES, encrypted_payload.data() + len, GCM_TAG_BYTES);
    std::memcpy(out_buffer + GCM_NONCE_BYTES + GCM_TAG_BYTES, encrypted_payload.data(), len);

    *out_len = len + GCM_NONCE_BYTES + GCM_TAG_BYTES;
    return true;
}

std::optional<std::vector<uint8_t>> aes_gcm_encrypt(
    const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& aes_key,
    CryptoBackend& backend)
{
    if (aes_key.size() != AES_KEY_BYTES) {
        LOG_E(TAG, "Invalid key size: expected %u, got %u",
              static_cast<unsigned>(AES_KEY_BYTES),
              static_cast<unsigned>(aes_key.size()));
        return std::nullopt;
    }

    size_t out_len = plaintext.size() + GCM_NONCE_BYTES + GCM_TAG_BYTES;
    std::vector<uint8_t> out_buffer(out_len);

    const bool ok = aes_gcm_encrypt(aes_key.data(), plaintext.data(), plaintext.size(),
                                    out_buffer.data(), &out_len, backend);
    if (!ok) {
        return std::nullopt;
    }

    out_buffer.resize(out_len);
    return out_buffer;
}

} // namespace encryption
