#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <cstring>
#include <algorithm>

namespace Protocol {
namespace WifiConfig {

    // Unique Message Type ID representing Upload/Sync of all Wi-Fi configurations
    constexpr uint8_t MSG_TYPE_WIFI_CONFIG = 0xA5;

    // Single Wi-Fi configuration entry
    struct Item {
        std::string ssid;
        std::string password;
        int32_t priority = 0; // Connection priority (higher value = higher priority)
    };

    // ========================================================================
    // Internal Serialization Engine
    // ========================================================================
    namespace Detail {

        inline bool is_wifi_config_raw(const uint8_t* data, size_t len) {
            return (data != nullptr && len >= 1 && data[0] == MSG_TYPE_WIFI_CONFIG);
        }

        inline std::vector<uint8_t> serialize_configs(const std::vector<Item>& configs) {
            std::vector<uint8_t> buffer;
            buffer.reserve(32 + configs.size() * 64);

            // 1. Message Type
            buffer.push_back(MSG_TYPE_WIFI_CONFIG);

            // 2. Number of configurations (2 bytes Little-Endian)
            uint16_t count = static_cast<uint16_t>(std::min(configs.size(), static_cast<size_t>(65535)));
            buffer.push_back(static_cast<uint8_t>(count & 0xFF));
            buffer.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));

            // 3. Serialize each item
            for (size_t i = 0; i < count; ++i) {
                const auto& item = configs[i];

                // SSID (1 byte length + string bytes)
                uint8_t ssid_len = static_cast<uint8_t>(std::min(item.ssid.size(), static_cast<size_t>(255)));
                buffer.push_back(ssid_len);
                buffer.insert(buffer.end(), item.ssid.begin(), item.ssid.begin() + ssid_len);

                // Password (1 byte length + string bytes)
                uint8_t pass_len = static_cast<uint8_t>(std::min(item.password.size(), static_cast<size_t>(255)));
                buffer.push_back(pass_len);
                buffer.insert(buffer.end(), item.password.begin(), item.password.begin() + pass_len);

                // Priority (4 bytes Little-Endian)
                uint32_t p = static_cast<uint32_t>(item.priority);
                buffer.push_back(static_cast<uint8_t>(p & 0xFF));
                buffer.push_back(static_cast<uint8_t>((p >> 8) & 0xFF));
                buffer.push_back(static_cast<uint8_t>((p >> 16) & 0xFF));
                buffer.push_back(static_cast<uint8_t>((p >> 24) & 0xFF));
            }

            return buffer;
        }

        inline std::optional<std::vector<Item>> deserialize_configs(const uint8_t* data, size_t len) {
            // Minimum size: 1 (type) + 2 (count) = 3 bytes
            if (!is_wifi_config_raw(data, len) || len < 3) {
                return std::nullopt;
            }

            uint16_t count = static_cast<uint16_t>(data[1]) |
                            (static_cast<uint16_t>(data[2]) << 8);

            size_t offset = 3;
            std::vector<Item> result;
            result.reserve(count);

            for (uint16_t i = 0; i < count; ++i) {
                Item item;

                // 1. Read SSID
                if (offset >= len) return std::nullopt;
                uint8_t ssid_len = data[offset++];
                if (offset + ssid_len > len) return std::nullopt;
                item.ssid.assign(reinterpret_cast<const char*>(data + offset), ssid_len);
                offset += ssid_len;

                // 2. Read Password
                if (offset >= len) return std::nullopt;
                uint8_t pass_len = data[offset++];
                if (offset + pass_len > len) return std::nullopt;
                item.password.assign(reinterpret_cast<const char*>(data + offset), pass_len);
                offset += pass_len;

                // 3. Read Priority (4 bytes Little-Endian)
                if (offset + 4 > len) return std::nullopt;
                uint32_t p = static_cast<uint32_t>(data[offset]) |
                            (static_cast<uint32_t>(data[offset + 1]) << 8) |
                            (static_cast<uint32_t>(data[offset + 2]) << 16) |
                            (static_cast<uint32_t>(data[offset + 3]) << 24);
                item.priority = static_cast<int32_t>(p);
                offset += 4;

                result.push_back(std::move(item));
            }

            return result;
        }

    } // namespace Detail

    // ========================================================================
    // CLIENT API (ESP32: Uploading configurations to Server)
    // ========================================================================
    namespace Client {

        /**
         * @brief Encodes the full list of Wi-Fi configurations to upload to the server
         */
        inline std::vector<uint8_t> encode(const std::vector<Item>& configs) {
            return Detail::serialize_configs(configs);
        }

        /**
         * @brief (Optional) If the server pushes Wi-Fi configurations down to the client
         */
        inline bool is_wifi_config(const uint8_t* data, size_t len) {
            return Detail::is_wifi_config_raw(data, len);
        }

        inline bool is_wifi_config(const std::vector<uint8_t>& buffer) {
            return Detail::is_wifi_config_raw(buffer.data(), buffer.size());
        }

        inline std::optional<std::vector<Item>> decode(const uint8_t* data, size_t len) {
            return Detail::deserialize_configs(data, len);
        }

        inline std::optional<std::vector<Item>> decode(const std::vector<uint8_t>& buffer) {
            return Detail::deserialize_configs(buffer.data(), buffer.size());
        }

    } // namespace Client

    // ========================================================================
    // SERVER API (Server: Receiving and decoding client Wi-Fi configurations)
    // ========================================================================
    namespace Server {

        /**
         * @brief Checks if an incoming client packet contains uploaded Wi-Fi configurations
         */
        inline bool is_wifi_config(const uint8_t* data, size_t len) {
            return Detail::is_wifi_config_raw(data, len);
        }

        inline bool is_wifi_config(const std::vector<uint8_t>& buffer) {
            return Detail::is_wifi_config_raw(buffer.data(), buffer.size());
        }

        /**
         * @brief Decodes the list of Wi-Fi configurations uploaded by the client
         */
        inline std::optional<std::vector<Item>> decode(const uint8_t* data, size_t len) {
            return Detail::deserialize_configs(data, len);
        }

        inline std::optional<std::vector<Item>> decode(const std::vector<uint8_t>& buffer) {
            return Detail::deserialize_configs(buffer.data(), buffer.size());
        }

        /**
         * @brief (Optional) Server can also encode configurations to sync to the device
         */
        inline std::vector<uint8_t> encode(const std::vector<Item>& configs) {
            return Detail::serialize_configs(configs);
        }

    } // namespace Server

} // namespace WifiConfig
} // namespace Protocol