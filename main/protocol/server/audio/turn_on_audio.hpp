#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <string>

namespace Protocol {
namespace RecordControl {

    // Unique Message Type ID
    constexpr uint8_t MSG_TYPE_RECORD_CTRL = 0xA4;

    enum class RecordCommand : uint8_t {
        STOP  = 0,
        START = 1
    };

    inline const char* to_string(RecordCommand cmd) {
        switch (cmd) {
            case RecordCommand::STOP:  return "STOP";
            case RecordCommand::START: return "START";
            default:                   return "UNKNOWN";
        }
    }

    struct RecordControlInfo {
        RecordCommand command = RecordCommand::STOP;
        uint32_t timestamp_ms = 0;
    };

    // ========================================================================
    // SERVER API: Server initiates and encodes commands to send to ESP32
    // ========================================================================
    namespace Server {

        /**
         * @brief Server serializes RecordControlInfo to send to the client
         */
        inline std::vector<uint8_t> encode(const RecordControlInfo& info) {
            std::vector<uint8_t> buffer;
            buffer.reserve(6);

            buffer.push_back(MSG_TYPE_RECORD_CTRL);
            buffer.push_back(static_cast<uint8_t>(info.command));

            // Timestamp (4 bytes Little-Endian)
            buffer.push_back(static_cast<uint8_t>(info.timestamp_ms & 0xFF));
            buffer.push_back(static_cast<uint8_t>((info.timestamp_ms >> 8) & 0xFF));
            buffer.push_back(static_cast<uint8_t>((info.timestamp_ms >> 16) & 0xFF));
            buffer.push_back(static_cast<uint8_t>((info.timestamp_ms >> 24) & 0xFF));

            return buffer;
        }

        inline std::vector<uint8_t> encode(RecordCommand cmd, uint32_t timestamp_ms = 0) {
            return encode(RecordControlInfo{cmd, timestamp_ms});
        }

    } // namespace Server

    // ========================================================================
    // CLIENT API: ESP32 client checks and decodes incoming commands
    // ========================================================================
    namespace Client {

        /**
         * @brief Checks whether the received packet is a Record Control command
         */
        inline bool is_record_control(const uint8_t* data, size_t len) {
            return (data != nullptr && len >= 1 && data[0] == MSG_TYPE_RECORD_CTRL);
        }

        inline bool is_record_control(const std::vector<uint8_t>& buffer) {
            return is_record_control(buffer.data(), buffer.size());
        }

        /**
         * @brief Client decodes the server's command
         */
        inline std::optional<RecordControlInfo> decode(const uint8_t* data, size_t len) {
            constexpr size_t PACKET_SIZE = 6;
            if (!is_record_control(data, len) || len < PACKET_SIZE) {
                return std::nullopt;
            }

            RecordControlInfo info;
            uint8_t raw_cmd = data[1];
            if (raw_cmd > 1) {
                return std::nullopt; // Corrupt enum
            }
            info.command = static_cast<RecordCommand>(raw_cmd);

            info.timestamp_ms = static_cast<uint32_t>(data[2]) |
                               (static_cast<uint32_t>(data[3]) << 8) |
                               (static_cast<uint32_t>(data[4]) << 16) |
                               (static_cast<uint32_t>(data[5]) << 24);

            return info;
        }

        inline std::optional<RecordControlInfo> decode(const std::vector<uint8_t>& buffer) {
            return decode(buffer.data(), buffer.size());
        }

    } // namespace Client

} // namespace RecordControl
} // namespace Protocol