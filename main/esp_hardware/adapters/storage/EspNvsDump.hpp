#pragma once

// One-shot boot diagnostic: list every key/value stored in the NVS partition.
// Secrets, passwords, tokens and Wi-Fi credential blobs are redacted by
// default. Set NVS_DUMP_SHOW_SENSITIVE=1 locally if the raw bytes are really
// needed in the log.
//
// Output example:
//   I NVS_DUMP: [storage] device_id type=str len=15 value="esp32s3-792814"
//   I NVS_DUMP: [storage] device_secret type=blob len=32 <redacted>
//   I NVS_DUMP: --- NVS dump complete: 7 entries ---

#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#ifndef NVS_DUMP_SHOW_SENSITIVE
#define NVS_DUMP_SHOW_SENSITIVE 0
#endif

namespace adapters {
namespace nvs_dump {

inline const char* typeName(nvs_type_t type) {
    switch (type) {
        case NVS_TYPE_U8:     return "u8";
        case NVS_TYPE_I8:     return "i8";
        case NVS_TYPE_U16:    return "u16";
        case NVS_TYPE_I16:    return "i16";
        case NVS_TYPE_U32:    return "u32";
        case NVS_TYPE_I32:    return "i32";
        case NVS_TYPE_U64:    return "u64";
        case NVS_TYPE_I64:    return "i64";
        case NVS_TYPE_FLOAT:  return "float";
        case NVS_TYPE_DOUBLE: return "double";
        case NVS_TYPE_STR:    return "str";
        case NVS_TYPE_BLOB:   return "blob";
        default:              return "any";
    }
}

inline bool isSensitive(const char* ns, const char* key) {
#if NVS_DUMP_SHOW_SENSITIVE
    (void)ns;
    (void)key;
    return false;
#else
    std::string s(ns);
    s += '/';
    s += key;
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    static const char* kTerms[] = {
        "secret", "password", "passwd", "pswd", "pass",
        "token", "pin", "key", "hash", "pmk",
    };
    for (const char* term : kTerms) {
        if (s.find(term) != std::string::npos) return true;
    }
    // The wifi_manager stores whole wifi_network_t records (SSID + password)
    // as blobs; never print those raw.
    if (std::strcmp(ns, "wifi_mgr") == 0) return true;
    return false;
#endif
}

inline std::string toHex(const uint8_t* data, std::size_t len, std::size_t max_bytes = 32) {
    static const char kDigits[] = "0123456789abcdef";
    const std::size_t shown = len < max_bytes ? len : max_bytes;
    std::string out;
    out.reserve(shown * 2 + 4);
    for (std::size_t i = 0; i < shown; ++i) {
        out.push_back(kDigits[data[i] >> 4]);
        out.push_back(kDigits[data[i] & 0x0F]);
    }
    if (len > shown) out += "...";
    return out;
}

inline std::string printable(const char* data, std::size_t len) {
    const std::size_t shown = len < 64 ? len : 64;
    std::string out;
    out.reserve(shown);
    for (std::size_t i = 0; i < shown; ++i) {
        const unsigned char c = static_cast<unsigned char>(data[i]);
        if (c == '\0') break;
        out.push_back(std::isprint(c) ? static_cast<char>(c) : '.');
    }
    if (len > shown) out += "...";
    return out;
}

inline void logValue(nvs_handle_t handle, const nvs_entry_info_t& info, bool sensitive) {
    static const char* TAG = "NVS_DUMP";

    switch (info.type) {
        case NVS_TYPE_STR: {
            std::size_t len = 0;
            if (nvs_get_str(handle, info.key, nullptr, &len) != ESP_OK) break;
            std::string buf(len, '\0');
            if (nvs_get_str(handle, info.key, buf.data(), &len) != ESP_OK) break;
            if (sensitive) {
                ESP_LOGI(TAG, "[%s] %s type=str len=%u <redacted>",
                         info.namespace_name, info.key, static_cast<unsigned>(len));
            } else {
                ESP_LOGI(TAG, "[%s] %s type=str len=%u value=\"%s\"",
                         info.namespace_name, info.key, static_cast<unsigned>(len),
                         printable(buf.data(), buf.size()).c_str());
            }
            break;
        }
        case NVS_TYPE_BLOB: {
            std::size_t len = 0;
            if (nvs_get_blob(handle, info.key, nullptr, &len) != ESP_OK) break;
            std::vector<uint8_t> buf(len);
            if (nvs_get_blob(handle, info.key, buf.data(), &len) != ESP_OK) break;
            if (sensitive) {
                ESP_LOGI(TAG, "[%s] %s type=blob len=%u <redacted>",
                         info.namespace_name, info.key, static_cast<unsigned>(len));
            } else {
                ESP_LOGI(TAG, "[%s] %s type=blob len=%u hex=%s",
                         info.namespace_name, info.key, static_cast<unsigned>(len),
                         toHex(buf.data(), buf.size()).c_str());
            }
            break;
        }
        case NVS_TYPE_U8: {
            uint8_t v = 0;
            if (nvs_get_u8(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=u8 value=%u", info.namespace_name,
                         info.key, static_cast<unsigned>(v));
            }
            break;
        }
        case NVS_TYPE_I8: {
            int8_t v = 0;
            if (nvs_get_i8(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=i8 value=%d", info.namespace_name,
                         info.key, static_cast<int>(v));
            }
            break;
        }
        case NVS_TYPE_U16: {
            uint16_t v = 0;
            if (nvs_get_u16(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=u16 value=%u", info.namespace_name,
                         info.key, static_cast<unsigned>(v));
            }
            break;
        }
        case NVS_TYPE_I16: {
            int16_t v = 0;
            if (nvs_get_i16(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=i16 value=%d", info.namespace_name,
                         info.key, static_cast<int>(v));
            }
            break;
        }
        case NVS_TYPE_U32: {
            uint32_t v = 0;
            if (nvs_get_u32(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=u32 value=%lu", info.namespace_name,
                         info.key, static_cast<unsigned long>(v));
            }
            break;
        }
        case NVS_TYPE_I32: {
            int32_t v = 0;
            if (nvs_get_i32(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=i32 value=%ld", info.namespace_name,
                         info.key, static_cast<long>(v));
            }
            break;
        }
        case NVS_TYPE_U64: {
            uint64_t v = 0;
            if (nvs_get_u64(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=u64 value=%llu", info.namespace_name,
                         info.key, static_cast<unsigned long long>(v));
            }
            break;
        }
        case NVS_TYPE_I64: {
            int64_t v = 0;
            if (nvs_get_i64(handle, info.key, &v) == ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=i64 value=%lld", info.namespace_name,
                         info.key, static_cast<long long>(v));
            }
            break;
        }
        default:
            ESP_LOGI(TAG, "[%s] %s type=%s <not decoded>", info.namespace_name,
                     info.key, typeName(info.type));
            break;
    }
}

inline void dumpNvsContents() {
    static const char* TAG = "NVS_DUMP";

    const esp_err_t init = nvs_flash_init();
    if (init != ESP_OK) {
        ESP_LOGW(TAG, "nvs_flash_init failed: %s", esp_err_to_name(init));
        return;
    }

    nvs_iterator_t it = nullptr;
    esp_err_t err = nvs_entry_find(NVS_DEFAULT_PART_NAME, nullptr, NVS_TYPE_ANY, &it);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_entry_find failed: %s (partition '%s')",
                 esp_err_to_name(err), NVS_DEFAULT_PART_NAME);
        return;
    }

    ESP_LOGI(TAG, "--- NVS contents (partition '%s') ---", NVS_DEFAULT_PART_NAME);
    std::size_t count = 0;
    while (err == ESP_OK) {
        nvs_entry_info_t info{};
        if (nvs_entry_info(it, &info) == ESP_OK) {
            ++count;
            nvs_handle_t handle;
            if (nvs_open_from_partition(NVS_DEFAULT_PART_NAME, info.namespace_name,
                                        NVS_READONLY, &handle) != ESP_OK) {
                ESP_LOGI(TAG, "[%s] %s type=%s <open failed>",
                         info.namespace_name, info.key, typeName(info.type));
            } else {
                logValue(handle, info, isSensitive(info.namespace_name, info.key));
                nvs_close(handle);
            }
        }
        err = nvs_entry_next(&it);
    }
    if (it != nullptr) {
        nvs_release_iterator(it);
    }
    ESP_LOGI(TAG, "--- NVS dump complete: %u entries ---", static_cast<unsigned>(count));
}

}  // namespace nvs_dump

inline void dumpNvsContents() { nvs_dump::dumpNvsContents(); }

}  // namespace adapters
