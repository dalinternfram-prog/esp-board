#pragma once

// Adapter: exposes the existing `encryption::` module through the core ICrypto port.
// Keeps mbedTLS/PSA details out of the domain core.

#include "core/ports.hpp"

#include "crypto.hpp"

namespace adapters {

class EspCrypto : public core::ICrypto {
public:
    std::optional<core::Bytes> generatePublicKey() override {
        return encryption::ecdh_step1_generate_my_key();
    }

    std::optional<core::Bytes> computeSharedSecret(const core::Bytes& peer_public_key) override {
        return encryption::ecdh_step2_compute_shared_secret(peer_public_key);
    }

    std::optional<core::Bytes> encrypt(const core::Bytes& plaintext, const core::Bytes& key) override {
        return encryption::aes_gcm_encrypt(plaintext, key);
    }

    std::optional<core::Bytes> decrypt(const core::Bytes& packet, const core::Bytes& key) override {
        return encryption::aes_gcm_decrypt(packet, key);
    }
};

} // namespace adapters
