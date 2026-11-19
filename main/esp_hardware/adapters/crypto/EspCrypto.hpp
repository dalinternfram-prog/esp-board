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

    std::optional<core::Bytes> sha256(const core::Bytes& data) override {
        if (psa_crypto_init() != PSA_SUCCESS) {
            return std::nullopt;
        }
        core::Bytes out(32);
        std::size_t out_len = 0;
        if (psa_hash_compute(PSA_ALG_SHA_256, data.data(), data.size(),
                             out.data(), out.size(), &out_len) != PSA_SUCCESS ||
            out_len != out.size()) {
            return std::nullopt;
        }
        return out;
    }

    std::optional<core::Bytes> hmacSha256(const core::Bytes& key,
                                          const core::Bytes& data) override {
        if (psa_crypto_init() != PSA_SUCCESS || key.empty()) {
            return std::nullopt;
        }

        psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
        psa_set_key_bits(&attr, key.size() * 8);
        psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
        psa_set_key_algorithm(&attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));

        psa_key_id_t key_id = 0;
        if (psa_import_key(&attr, key.data(), key.size(), &key_id) != PSA_SUCCESS) {
            psa_reset_key_attributes(&attr);
            return std::nullopt;
        }

        core::Bytes out(32);
        std::size_t out_len = 0;
        const psa_status_t status =
            psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                            data.data(), data.size(),
                            out.data(), out.size(), &out_len);
        psa_destroy_key(key_id);
        psa_reset_key_attributes(&attr);

        if (status != PSA_SUCCESS || out_len != out.size()) {
            return std::nullopt;
        }
        return out;
    }

private:
    EspCryptoBackend       backend_;
    encryption::EcdhHandshake ecdh_;
};

} // namespace adapters
