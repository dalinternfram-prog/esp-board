#include "crypto.hpp"

#include <cstring>

namespace encryption {

bool aes_gcm_decrypt(const uint8_t key[AES_KEY_BYTES],
                     const uint8_t* packet, size_t packet_len,
                     uint8_t* plaintext_out, size_t* plaintext_len,
                     CryptoBackend& backend)
{
    if (!backend.init()) {
        return false;
    }

    if (packet_len < GCM_NONCE_BYTES + GCM_TAG_BYTES) {
        LOG_E(TAG, "Packet too short");
        return false;
    }

    const size_t ciphertext_len = packet_len - GCM_NONCE_BYTES - GCM_TAG_BYTES;
    if (*plaintext_len < ciphertext_len) {
        LOG_E(TAG, "Output buffer too small");
        return false;
    }

    // Packet layout: [12B nonce | 16B tag | ciphertext]
    const uint8_t* nonce = packet;
    const uint8_t* tag = packet + GCM_NONCE_BYTES;
    const uint8_t* ciphertext = packet + GCM_NONCE_BYTES + GCM_TAG_BYTES;

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, AES_KEY_BYTES * 8);

    psa_key_id_t key_id;
    psa_status_t status = psa_import_key(&attributes, key, AES_KEY_BYTES, &key_id);
    if (status != PSA_SUCCESS) {
        LOG_E(TAG, "Failed to import key: %d", static_cast<int>(status));
        return false;
    }

    // PSA expects the tag concatenated immediately after the ciphertext.
    std::vector<uint8_t> combined(ciphertext_len + GCM_TAG_BYTES);
    std::memcpy(combined.data(), ciphertext, ciphertext_len);
    std::memcpy(combined.data() + ciphertext_len, tag, GCM_TAG_BYTES);

    size_t decrypted_size = 0;
    status = psa_aead_decrypt(
        key_id,
        PSA_ALG_GCM,
        nonce, GCM_NONCE_BYTES,
        nullptr, 0,  // no AAD
        combined.data(), combined.size(),
        plaintext_out, *plaintext_len,
        &decrypted_size);

    psa_destroy_key(key_id);

    if (status != PSA_SUCCESS) {
        LOG_E(TAG, "Decryption or auth tag verification failed! Error: %d",
              static_cast<int>(status));
        return false;
    }

    *plaintext_len = decrypted_size;
    return true;
}

std::optional<std::vector<uint8_t>> aes_gcm_decrypt(const std::vector<uint8_t>& packet,
                                                    const std::vector<uint8_t>& key,
                                                    CryptoBackend& backend)
{
    if (key.size() != AES_KEY_BYTES) {
        LOG_E(TAG, "Invalid key size: expected %u, got %u",
              static_cast<unsigned>(AES_KEY_BYTES),
              static_cast<unsigned>(key.size()));
        return std::nullopt;
    }

    if (packet.size() < GCM_NONCE_BYTES + GCM_TAG_BYTES) {
        LOG_E(TAG, "Packet too short");
        return std::nullopt;
    }

    size_t plaintext_len = packet.size() - GCM_NONCE_BYTES - GCM_TAG_BYTES;
    std::vector<uint8_t> plaintext_out(plaintext_len);

    const bool ok = aes_gcm_decrypt(key.data(), packet.data(), packet.size(),
                                    plaintext_out.data(), &plaintext_len, backend);
    if (!ok) {
        return std::nullopt;
    }

    plaintext_out.resize(plaintext_len);
    return plaintext_out;
}

} // namespace encryption
