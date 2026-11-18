#pragma once

// Adapter: core::ICrypto over the portable PSA crypto module.
// The thin platform wrapper (PSA init + RNG) is implemented here; all protocol
// logic (AES-GCM layout, ECDH handshake) stays in general/crypto.

#include "interface.hpp"

#include "crypto.hpp"

namespace adapters {

class EspCryptoBackend : public encryption::CryptoBackend {
public:
    bool init() override {
        static bool initialized = false;
        if (!initialized) {
            if (psa_crypto_init() != PSA_SUCCESS) {
                return false;
            }
            initialized = true;
        }
        return true;
    }

    void fillRandom(uint8_t* output, std::size_t len) override {
        psa_generate_random(output, len);
    }
};

class EspCrypto : public core::ICrypto {
public:
    std::optional<core::Bytes> generatePublicKey() override {
        return encryption::ecdh_step1_generate_my_key(backend_);
    }

    std::optional<core::Bytes> computeSharedSecret(const core::Bytes& peer_public_key) override {
        return encryption::ecdh_step2_compute_shared_secret(peer_public_key, backend_);
    }

    std::optional<core::Bytes> encrypt(const core::Bytes& plaintext,
                                       const core::Bytes& key) override {
        return encryption::aes_gcm_encrypt(plaintext, key, backend_);
    }

    std::optional<core::Bytes> decrypt(const core::Bytes& packet,
                                       const core::Bytes& key) override {
        return encryption::aes_gcm_decrypt(packet, key, backend_);
    }

private:
    EspCryptoBackend backend_;
};

} // namespace adapters
