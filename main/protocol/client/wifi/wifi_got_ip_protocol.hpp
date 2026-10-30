#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <cstring>
#include <algorithm>

namespace Protocol {
namespace Wifi {

    // Unique Message Type ID representing the GOT_IP event
    constexpr uint8_t MSG_TYPE_GOT_IP = 0xA1;

    // Cross-platform container for IP status information
    struct GotIpInfo {
        std::string ip;
        std::string gateway;
        std::string netmask;
        std::string dns;
        std::string mac;
        int32_t quality = 0; // Signal quality percentage (0 - 100)
    };

    // ------------------------------------------------------------------------
    // 1. Packet Type Identifier (Used by Server / Receiver)
    // ------------------------------------------------------------------------

    /**
     * @brief Checks if the raw byte payload represents a GOT_IP event packet
     * @param data Pointer to raw byte buffer
     * @param len  Length of the buffer
     * @return true if the packet starts with MSG_TYPE_GOT_IP, false otherwise
     */
    inline bool is_got_ip(const uint8_t* data, size_t len) {
        return (data != nullptr && len >= 1 && data[0] == MSG_TYPE_GOT_IP);
    }

    inline bool is_got_ip(const std::vector<uint8_t>& buffer) {
        return is_got_ip(buffer.data(), buffer.size());
    }

    // ------------------------------------------------------------------------
    // 2. Encoder (Used by Client / ESP32)
    // ------------------------------------------------------------------------

    /**
     * @brief Serializes GotIpInfo into an endian-safe byte payload
     * Layout:
     *   [0]      : uint8_t  msg_type (0xA1)
     *   [1..4]   : int32_t  quality (little-endian)
     *   [5]      : uint8_t  ip_len
     *   [..]     : char[]   ip string
     *   Followed by gateway, netmask, dns, and mac in identical length-prefixed format.
     */
    inline std::vector<uint8_t> encode_got_ip(const GotIpInfo& info) {
        std::vector<uint8_t> buffer;
        buffer.reserve(128); // Pre-allocate to prevent reallocations

        // 1. Message Type
        buffer.push_back(MSG_TYPE_GOT_IP);

        // 2. Quality (4 bytes, Little-Endian)
        uint32_t q = static_cast<uint32_t>(info.quality);
        buffer.push_back(static_cast<uint8_t>(q & 0xFF));
        buffer.push_back(static_cast<uint8_t>((q >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((q >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((q >> 24) & 0xFF));

        // Helper to append length-prefixed strings
        auto append_string = [&buffer](const std::string& str) {
            uint8_t str_len = static_cast<uint8_t>(std::min(str.size(), static_cast<size_t>(255)));
            buffer.push_back(str_len);
            buffer.insert(buffer.end(), str.data(), str.data() + str_len);
        };

        // 3. String Fields
        append_string(info.ip);
        append_string(info.gateway);
        append_string(info.netmask);
        append_string(info.dns);
        append_string(info.mac);

        return buffer;
    }

    // ------------------------------------------------------------------------
    // 3. Decoder (Used by Server / Receiver)
    // ------------------------------------------------------------------------

    /**
     * @brief Deserializes a raw byte buffer back into GotIpInfo
     * @param data Pointer to raw byte buffer
     * @param len  Length of the buffer
     * @return std::optional containing GotIpInfo if valid, std::nullopt if corrupted/truncated
     */
    inline std::optional<GotIpInfo> decode_got_ip(const uint8_t* data, size_t len) {
        // Minimum valid payload: 1 (type) + 4 (quality) + 5 (1 byte per 5 string lengths) = 10 bytes
        if (!is_got_ip(data, len) || len < 10) {
            return std::nullopt;
        }

        GotIpInfo info;
        size_t offset = 1; // Skip MSG_TYPE_GOT_IP

        // Read Quality (4 bytes Little-Endian)
        uint32_t q = static_cast<uint32_t>(data[offset]) |
                    (static_cast<uint32_t>(data[offset + 1]) << 8) |
                    (static_cast<uint32_t>(data[offset + 2]) << 16) |
                    (static_cast<uint32_t>(data[offset + 3]) << 24);
        info.quality = static_cast<int32_t>(q);
        offset += 4;

        // Helper to read length-prefixed string safely
        auto read_string = [data, len, &offset](std::string& out) -> bool {
            if (offset >= len) return false;
            uint8_t str_len = data[offset++];
            if (offset + str_len > len) return false; // Buffer overflow guard
            out.assign(reinterpret_cast<const char*>(data + offset), str_len);
            offset += str_len;
            return true;
        };

        if (!read_string(info.ip))      return std::nullopt;
        if (!read_string(info.gateway)) return std::nullopt;
        if (!read_string(info.netmask)) return std::nullopt;
        if (!read_string(info.dns))     return std::nullopt;
        if (!read_string(info.mac))     return std::nullopt;

        return info;
    }

    inline std::optional<GotIpInfo> decode_got_ip(const std::vector<uint8_t>& buffer) {
        return decode_got_ip(buffer.data(), buffer.size());
    }

} // namespace Wifi
} // namespace Protocol