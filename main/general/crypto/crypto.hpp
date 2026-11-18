#pragma once

// Portable crypto primitives (PSA-backed AES-256-GCM + ECDH P-256).
// mbedTLS/PSA is a portable library; platform-specific init and randomness are
// injected through CryptoBackend (implemented by the hardware layer).

#include "psa/crypto.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "general/log.hpp"

namespace encryption {

static const char* TAG = "AES_GCM";

constexpr size_t AES_KEY_BYTES = 32;   // 256-bit AES key
constexpr size_t GCM_NONCE_BYTES = 12; // standard 12-byte nonce (IV)
constexpr size_t GCM_TAG_BYTES = 16;   // standard 16-byte auth tag

// Platform glue: one-time init + cryptographically secure randomness.
struct CryptoBackend {
    virtual ~CryptoBackend() = default;
    virtual bool init() = 0;
    virtual void fillRandom(uint8_t* output, std::size_t len) = 0;
};

// Encrypts plaintext using AES-256-GCM. Output layout: [12B nonce | 16B tag | ciphertext].
bool aes_gcm_encrypt(const uint8_t key[AES_KEY_BYTES],
                     const uint8_t* plaintext, size_t len,
                     uint8_t* out_buffer, size_t* out_len,
                     CryptoBackend& backend);

std::optional<std::vector<uint8_t>> aes_gcm_encrypt(const std::vector<uint8_t>& plaintext,
                                                    std::vector<uint8_t> aes_key,
                                                    CryptoBackend& backend);

// Decrypts a packet laid out as [12B nonce | 16B tag | ciphertext].
bool aes_gcm_decrypt(const uint8_t key[AES_KEY_BYTES],
                     const uint8_t* packet, size_t packet_len,
                     uint8_t* plaintext_out, size_t* plaintext_len,
                     CryptoBackend& backend);

std::optional<std::vector<uint8_t>> aes_gcm_decrypt(const std::vector<uint8_t>& packet,
                                                    std::vector<uint8_t> key,
                                                    CryptoBackend& backend);

// ECDH P-256 handshake.
std::optional<std::vector<uint8_t>> ecdh_step1_generate_my_key(CryptoBackend& backend);
std::optional<std::vector<uint8_t>> ecdh_step2_compute_shared_secret(
    const std::vector<uint8_t>& peer_public_key, CryptoBackend& backend);

} // namespace encryption
