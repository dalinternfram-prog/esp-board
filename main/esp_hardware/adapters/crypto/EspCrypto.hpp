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
        // psa_crypto_init() is idempotent and safe to call repeatedly, so no
        // local state is needed (and no cross-task race on a cached flag).
        return psa_crypto_init() == PSA_SUCCESS;
    }

    void fillRandom(uint8_t* output, std::size_t len) override {
        psa_generate_random(output, len);
    }
};

class EspCrypto : public core::ICrypto {
public:
    std::optional<core::Bytes> generatePublicKey() override {
        return ecdh_.generatePublicKey(backend_);
    }

    std::optional<core::Bytes> computeSharedSecret(const core::Bytes& peer_public_key) override {
        return ecdh_.computeSharedSecret(peer_public_key, backend_);
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
    EspCryptoBackend       backend_;
    encryption::EcdhHandshake ecdh_;
};

} // namespace adapters
