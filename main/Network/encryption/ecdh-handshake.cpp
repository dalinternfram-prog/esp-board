#include <esp_log.h>
#include <esp_random.h>
#include "psa/crypto.h"
#include <vector>
#include <optional>

static const char* TAG = "ECDH_SECURE";
static psa_key_id_t g_local_ecdh_key_id = 0;

/// Step 1: Generate local keypair and return our public key as a vector
std::optional<std::vector<uint8_t>> encrytion::ecdh_step1_generate_my_key() {
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    // Generate local keypair
    status = psa_generate_key(&attributes, &g_local_ecdh_key_id);
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "Failed to generate local ECDH key: %d", status);
        return std::nullopt;
    }

    // P-256 uncompressed public key max size is 65 bytes
    std::vector<uint8_t> pub_key(65);
    size_t pub_len = 0;

    status = psa_export_public_key(g_local_ecdh_key_id, pub_key.data(), pub_key.size(), &pub_len);
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "Failed to export local public key: %d", status);
        psa_destroy_key(g_local_ecdh_key_id);
        g_local_ecdh_key_id = 0;
        return std::nullopt;
    }

    // Shrink vector to match the actual exported public key size
    pub_key.resize(pub_len);
    return pub_key; // Ready to send over KCP!
}

/// Step 2: Take server's public vector, compute shared secret, and return it
std::optional<std::vector<uint8_t>> encrytion::ecdh_step2_compute_shared_secret(const std::vector<uint8_t>& peer_public_key) {
    if (g_local_ecdh_key_id == 0) {
        ESP_LOGE(TAG, "No active local ECDH key found!");
        return std::nullopt;
    }

    // P-256 shared secret is always 32 bytes
    std::vector<uint8_t> shared_secret(32);
    size_t secret_len = 0;

    psa_status_t status = psa_raw_key_agreement(
        PSA_ALG_ECDH,
        g_local_ecdh_key_id,
        peer_public_key.data(),
        peer_public_key.size(),
        shared_secret.data(),
        shared_secret.size(),
        &secret_len
    );

    // Clean up local private key immediately
    psa_destroy_key(g_local_ecdh_key_id);
    g_local_ecdh_key_id = 0;

    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "Failed to compute shared secret: %d", status);
        return std::nullopt;
    }

    shared_secret.resize(secret_len);
    ESP_LOGI(TAG, "Handshake complete! Shared secret derived successfully.");
    return shared_secret; // Ready to use as your AES-GCM key!
}