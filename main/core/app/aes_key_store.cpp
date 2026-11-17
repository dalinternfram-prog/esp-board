#include "crypto.hpp"

#include <vector>

#include "app_context.hpp"
#include "core/storage/storage.hpp"

namespace encryption {

// Loads the AES key from persistent storage through the IStorage port.
// Storage backend and logging are ports, so this logic is hardware-agnostic.
std::optional<std::vector<uint8_t>> load_aes_key_from_nvs()
{
    auto& log = app::context().logger;

    std::vector<uint8_t> key_out;
    if (!storage::nvs().get("aes_key", key_out)) {
        log.logf(core::LogLevel::Error,
                 "AES key does not exist or could not be read from storage");
        return std::nullopt;
    }

    if (key_out.size() != AES_KEY_BYTES) {
        log.logf(core::LogLevel::Error,
                 "Retrieved key size mismatch: expected %u, got %u",
                 static_cast<unsigned>(AES_KEY_BYTES),
                 static_cast<unsigned>(key_out.size()));
        return std::nullopt;
    }

    return key_out;
}

} // namespace encryption
