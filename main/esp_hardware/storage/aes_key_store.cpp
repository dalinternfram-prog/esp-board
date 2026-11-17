#include "crypto.hpp"

#include <cstring>

#include "esp_log.h"
#include "core/storage/storage.hpp"

namespace encryption {

// Loads the AES key from persistent storage through the IStorage port.
// The storage backend (NVS today) is an adapter; this code is storage-agnostic.
std::optional<std::vector<uint8_t>> load_aes_key_from_nvs()
{
    std::vector<uint8_t> key_out;
    if (!storage::nvs().get("aes_key", key_out)) {
        ESP_LOGE(TAG, "AES key does not exist or could not be read from storage");
        return std::nullopt;
    }

    if (key_out.size() != AES_KEY_BYTES) {
        ESP_LOGE(TAG, "Retrieved key size mismatch: expected %u, got %u",
                 static_cast<unsigned>(AES_KEY_BYTES),
                 static_cast<unsigned>(key_out.size()));
        return std::nullopt;
    }

    return key_out;
}

} // namespace encryption
