#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <cstring>

namespace Protocol {
namespace Audio {

    // Unique Message Type ID representing an Audio Frame packet
    constexpr uint8_t MSG_TYPE_AUDIO = 0xA3;

    // Cross-platform container for compressed audio payload with timestamp
    struct AudioPacket {
        uint32_t timestamp_ms = 0;  // Millisecond timestamp when audio was recorded
        std::vector<uint8_t> payload; // Compressed PCM frame (Opus / LC3)
    };

    // ------------------------------------------------------------------------
    // 1. Packet Identifier
    // ------------------------------------------------------------------------

    /**
     * @brief Checks if the incoming raw byte buffer is an audio frame packet
     * @param data Pointer to raw byte buffer
     * @param len  Length of the buffer
     * @return true if packet matches MSG_TYPE_AUDIO, false otherwise
     */
    inline bool is_audio(const uint8_t* data, size_t len) {
        return (data != nullptr && len >= 1 && data[0] == MSG_TYPE_AUDIO);
    }

    inline bool is_audio(const std::vector<uint8_t>& buffer) {
        return is_audio(buffer.data(), buffer.size());
    }

    // ------------------------------------------------------------------------
    // 2. Encoder (Used by Client / ESP32)
    // ------------------------------------------------------------------------

    /**
     * @brief Encodes compressed audio data and timestamp into an endian-safe payload
     * Layout:
     *   [0]      : uint8_t  msg_type (0xA3)
     *   [1..4]   : uint32_t timestamp_ms (little-endian)
     *   [5..8]   : uint32_t payload_len  (little-endian)
     *   [9..]    : raw compressed audio bytes
     */
    inline std::vector<uint8_t> encode_audio(const AudioPacket& packet) {
        std::vector<uint8_t> buffer;
        buffer.reserve(1 + 4 + 4 + packet.payload.size());

        // 1. Message Type
        buffer.push_back(MSG_TYPE_AUDIO);

        // 2. Timestamp (4 bytes, Little-Endian)
        buffer.push_back(static_cast<uint8_t>(packet.timestamp_ms & 0xFF));
        buffer.push_back(static_cast<uint8_t>((packet.timestamp_ms >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((packet.timestamp_ms >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((packet.timestamp_ms >> 24) & 0xFF));

        // 3. Payload Length (4 bytes, Little-Endian)
        uint32_t len = static_cast<uint32_t>(packet.payload.size());
        buffer.push_back(static_cast<uint8_t>(len & 0xFF));
        buffer.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));

        // 4. Raw Opus/LC3 compressed bytes
        buffer.insert(buffer.end(), packet.payload.begin(), packet.payload.end());

        return buffer;
    }

    // Convenient overload: encode directly from raw vector and timestamp
    inline std::vector<uint8_t> encode_audio(const std::vector<uint8_t>& compressed_pcm, uint32_t timestamp_ms) {
        return encode_audio(AudioPacket{timestamp_ms, compressed_pcm});
    }

    // ------------------------------------------------------------------------
    // 3. Decoder (Used by Server)
    // ------------------------------------------------------------------------

    /**
     * @brief Decodes raw network bytes back into timestamp and compressed audio payload
     * @param data Pointer to raw byte buffer
     * @param len  Length of buffer
     * @return std::optional containing AudioPacket if valid, std::nullopt if corrupted
     */
    inline std::optional<AudioPacket> decode_audio(const uint8_t* data, size_t len) {
        // Minimum valid frame: 1 (type) + 4 (timestamp) + 4 (payload_len) = 9 bytes
        constexpr size_t HEADER_SIZE = 9;
        if (!is_audio(data, len) || len < HEADER_SIZE) {
            return std::nullopt;
        }

        AudioPacket packet;

        // 1. Read Timestamp (4 bytes, Little-Endian)
        packet.timestamp_ms = static_cast<uint32_t>(data[1]) |
                             (static_cast<uint32_t>(data[2]) << 8) |
                             (static_cast<uint32_t>(data[3]) << 16) |
                             (static_cast<uint32_t>(data[4]) << 24);

        // 2. Read Payload Length (4 bytes, Little-Endian)
        uint32_t payload_len = static_cast<uint32_t>(data[5]) |
                              (static_cast<uint32_t>(data[6]) << 8) |
                              (static_cast<uint32_t>(data[7]) << 16) |
                              (static_cast<uint32_t>(data[8]) << 24);

        // Bounds check: ensure the buffer is complete and not truncated
        if (len < HEADER_SIZE + payload_len) {
            return std::nullopt;
        }

        // 3. Extract compressed audio frame
        const uint8_t* payload_start = data + HEADER_SIZE;
        packet.payload.assign(payload_start, payload_start + payload_len);

        return packet;
    }

    inline std::optional<AudioPacket> decode_audio(const std::vector<uint8_t>& buffer) {
        return decode_audio(buffer.data(), buffer.size());
    }

} // namespace Audio
} // namespace Protocol