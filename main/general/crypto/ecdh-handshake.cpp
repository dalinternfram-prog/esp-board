#include "crypto.hpp"

#include <optional>
#include <vector>

namespace encryption {

EcdhHandshake::~EcdhHandshake() {
    if (key_id_ != 0) {
        psa_destroy_key(key_id_);
        key_id_ = 0;
    }
}

// Step 1: generate the local keypair and return our public key.
std::optional<std::vector<uint8_t>> EcdhHandshake::generatePublicKey(CryptoBackend& backend) {
    if (!backend.init()) {
        LOG_E(TAG, "Crypto backend init failed");
        return std::nullopt;
    }

    // Destroy any previous (unconsumed) keypair: aborted handshakes must not
    // leak PSA key slots.
    if (key_id_ != 0) {
        psa_destroy_key(key_id_);
        key_id_ = 0;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    psa_status_t status = psa_generate_key(&attributes, &key_id_);
    if (status != PSA_SUCCESS) {
        LOG_E(TAG, "Failed to generate local ECDH key: %d", static_cast<int>(status));
        return std::nullopt;
    }

    // P-256 uncompressed public key max size is 65 bytes.
    std::vector<uint8_t> pub_key(65);
    size_t pub_len = 0;

    status = psa_export_public_key(key_id_, pub_key.data(), pub_key.size(), &pub_len);
    if (status != PSA_SUCCESS) {
        LOG_E(TAG, "Failed to export local public key: %d", static_cast<int>(status));
        psa_destroy_key(key_id_);
        key_id_ = 0;
        return std::nullopt;
    }

    pub_key.resize(pub_len);
    return pub_key;
}

// Step 2: take the server's public key, derive the shared secret, drop the key.
std::optional<std::vector<uint8_t>> EcdhHandshake::computeSharedSecret(
    const std::vector<uint8_t>& peer_public_key, CryptoBackend& backend)
{
    if (!backend.init()) {
        LOG_E(TAG, "Crypto backend init failed");
        return std::nullopt;
    }

    if (key_id_ == 0) {
        LOG_E(TAG, "No active local ECDH key found!");
        return std::nullopt;
    }

    // P-256 shared secret is always 32 bytes.
    std::vector<uint8_t> shared_secret(32);
    size_t secret_len = 0;

    psa_status_t status = psa_raw_key_agreement(
        PSA_ALG_ECDH,
        key_id_,
        peer_public_key.data(),
        peer_public_key.size(),
        shared_secret.data(),
        shared_secret.size(),
        &secret_len);

    if (status != PSA_SUCCESS) {
        // Keep the keypair: a stray/corrupt packet must not kill the pending
        // handshake. SessionManager retries and can still complete.
        LOG_E(TAG, "Failed to compute shared secret: %d", static_cast<int>(status));
        return std::nullopt;
    }

    psa_destroy_key(key_id_);
    key_id_ = 0;

    shared_secret.resize(secret_len);
    LOG_I(TAG, "Handshake complete! Shared secret derived successfully.");
    return shared_secret;
}

} // namespace encryption
