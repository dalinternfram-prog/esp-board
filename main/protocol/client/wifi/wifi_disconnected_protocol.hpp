#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <cstring>
#include <algorithm>

namespace Protocol {
namespace Wifi {

    // Unique Message Type ID representing the DISCONNECTED event
    constexpr uint8_t MSG_TYPE_DISCONNECTED = 0xA2;

    /**
     * @brief Cross-platform Wi-Fi disconnection reason codes.
     * Numeric values align with IEEE 802.11 standards and ESP-IDF definitions.
     */
    enum class DisconnectReason : uint16_t {
        UNKNOWN                      = 0,
        UNSPECIFIED                  = 1,
        PREV_AUTH_NOT_VALID          = 2,
        DEAUTH_LEAVING               = 3,
        DISASSOC_DUE_TO_INACTIVITY   = 4,
        DISASSOC_AP_BUSY             = 5,
        CLASS2_FRAME_FROM_NONAUTH_STA = 6,
        CLASS3_FRAME_FROM_NONASSOC_STA = 7,
        DISASSOC_STA_HAS_LEFT        = 8,
        STA_REQ_ASSOC_WITHOUT_AUTH   = 9,
        PWR_CAPABILITY_NOT_VALID     = 10,
        SUPPORTED_CHANNEL_NOT_VALID  = 11,
        BSS_TRANSITION_DISASSOC      = 12,
        INVALID_ELEMENT              = 13,
        MIC_FAILURE                  = 14,
        FOURWAY_HANDSHAKE_TIMEOUT    = 15,
        GROUP_KEY_UPDATE_TIMEOUT     = 16,
        IE_IN_4WAY_DIFFERS           = 17,
        GROUP_CIPHER_INVALID         = 18,
        PAIRWISE_CIPHER_INVALID      = 19,
        AKMP_INVALID                 = 20,
        UNSUPP_RSN_IE_VERSION        = 21,
        INVALID_RSN_IE_CAP           = 22,
        IEEE802_1X_AUTH_FAILED       = 23,
        CIPHER_SUITE_REJECTED        = 24,

        // ESP-IDF Specific Extensions (>= 200)
        BEACON_TIMEOUT               = 200,
        NO_AP_FOUND                  = 201,
        AUTH_FAIL                    = 202,
        ASSOC_FAIL                   = 203,
        HANDSHAKE_TIMEOUT            = 204,
        CONNECTION_FAIL              = 205,
        AP_TSF_RESET                 = 206,
        ROAMING                      = 207,
        ASSOC_COMEBACK_TIME_TOO_LONG = 208,
        SA_QUERY_TIMEOUT             = 209
    };

    /**
     * @brief Converts a DisconnectReason enum to a human-readable string (Used by Server)
     */
    inline const char* to_string(DisconnectReason reason) {
        switch (reason) {
            case DisconnectReason::UNSPECIFIED:                  return "Unspecified error";
            case DisconnectReason::PREV_AUTH_NOT_VALID:          return "Previous authentication no longer valid";
            case DisconnectReason::DEAUTH_LEAVING:               return "Deauthenticated because device is leaving";
            case DisconnectReason::DISASSOC_DUE_TO_INACTIVITY:   return "Disassociated due to inactivity";
            case DisconnectReason::DISASSOC_AP_BUSY:             return "Disassociated because AP is busy";
            case DisconnectReason::DISASSOC_STA_HAS_LEFT:        return "Disassociated because STA has left";
            case DisconnectReason::MIC_FAILURE:                  return "MIC failure detected";
            case DisconnectReason::FOURWAY_HANDSHAKE_TIMEOUT:    return "4-way handshake timeout (wrong password?)";
            case DisconnectReason::GROUP_KEY_UPDATE_TIMEOUT:     return "Group key handshake timeout";
            case DisconnectReason::IEEE802_1X_AUTH_FAILED:       return "802.1X authentication failed";
            case DisconnectReason::BEACON_TIMEOUT:               return "Beacon lost / AP signal drop (Beacon timeout)";
            case DisconnectReason::NO_AP_FOUND:                  return "No matching AP / SSID found";
            case DisconnectReason::AUTH_FAIL:                    return "Authentication failed";
            case DisconnectReason::ASSOC_FAIL:                   return "Association failed";
            case DisconnectReason::HANDSHAKE_TIMEOUT:            return "Handshake timeout";
            case DisconnectReason::CONNECTION_FAIL:              return "Connection failed";
            case DisconnectReason::ROAMING:                      return "Disconnected due to roaming";
            default:                                             return "Unknown / Unmapped reason";
        }
    }

    // Container for Disconnection payload
    struct DisconnectedInfo {
        std::string ssid;
        DisconnectReason reason = DisconnectReason::UNKNOWN;
        uint32_t timestamp_ms = 0; // Device uptime/timestamp when disconnect occurred
    };

    // ------------------------------------------------------------------------
    // 1. Packet Identifier
    // ------------------------------------------------------------------------

    inline bool is_disconnected(const uint8_t* data, size_t len) {
        return (data != nullptr && len >= 1 && data[0] == MSG_TYPE_DISCONNECTED);
    }

    inline bool is_disconnected(const std::vector<uint8_t>& buffer) {
        return is_disconnected(buffer.data(), buffer.size());
    }

    // ------------------------------------------------------------------------
    // 2. Encoder (Used by Client / ESP32)
    // ------------------------------------------------------------------------

    /**
     * @brief Serializes DisconnectedInfo into an endian-safe byte payload
     * Layout:
     *   [0]      : uint8_t  msg_type (0xA2)
     *   [1..2]   : uint16_t reason enum value (little-endian)
     *   [3..6]   : uint32_t timestamp_ms (little-endian)
     *   [7]      : uint8_t  ssid_len
     *   [8..]    : char[]   ssid string
     */
    inline std::vector<uint8_t> encode_disconnected(const DisconnectedInfo& info) {
        std::vector<uint8_t> buffer;
        buffer.reserve(48);

        // 1. Message Type
        buffer.push_back(MSG_TYPE_DISCONNECTED);

        // 2. Reason Code (2 bytes, Little-Endian)
        uint16_t r = static_cast<uint16_t>(info.reason);
        buffer.push_back(static_cast<uint8_t>(r & 0xFF));
        buffer.push_back(static_cast<uint8_t>((r >> 8) & 0xFF));

        // 3. Timestamp (4 bytes, Little-Endian)
        buffer.push_back(static_cast<uint8_t>(info.timestamp_ms & 0xFF));
        buffer.push_back(static_cast<uint8_t>((info.timestamp_ms >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((info.timestamp_ms >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((info.timestamp_ms >> 24) & 0xFF));

        // 4. Length-prefixed SSID
        uint8_t ssid_len = static_cast<uint8_t>(std::min(info.ssid.size(), static_cast<size_t>(255)));
        buffer.push_back(ssid_len);
        buffer.insert(buffer.end(), info.ssid.data(), info.ssid.data() + ssid_len);

        return buffer;
    }

    // ------------------------------------------------------------------------
    // 3. Decoder (Used by Server)
    // ------------------------------------------------------------------------

    inline std::optional<DisconnectedInfo> decode_disconnected(const uint8_t* data, size_t len) {
        // Minimum valid length: 1 (type) + 2 (reason) + 4 (timestamp) + 1 (ssid_len) = 8 bytes
        constexpr size_t MIN_SIZE = 8;
        if (!is_disconnected(data, len) || len < MIN_SIZE) {
            return std::nullopt;
        }

        DisconnectedInfo info;
        size_t offset = 1; // Skip MSG_TYPE_DISCONNECTED

        // 1. Read Reason (2 bytes Little-Endian)
        uint16_t r = static_cast<uint16_t>(data[offset]) |
                    (static_cast<uint16_t>(data[offset + 1]) << 8);
        info.reason = static_cast<DisconnectReason>(r);
        offset += 2;

        // 2. Read Timestamp (4 bytes Little-Endian)
        info.timestamp_ms = static_cast<uint32_t>(data[offset]) |
                           (static_cast<uint32_t>(data[offset + 1]) << 8) |
                           (static_cast<uint32_t>(data[offset + 2]) << 16) |
                           (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;

        // 3. Read SSID safely
        uint8_t ssid_len = data[offset++];
        if (offset + ssid_len > len) {
            return std::nullopt; // Buffer overflow guard
        }

        info.ssid.assign(reinterpret_cast<const char*>(data + offset), ssid_len);

        return info;
    }

    inline std::optional<DisconnectedInfo> decode_disconnected(const std::vector<uint8_t>& buffer) {
        return decode_disconnected(buffer.data(), buffer.size());
    }

} // namespace Wifi
} // namespace Protocol