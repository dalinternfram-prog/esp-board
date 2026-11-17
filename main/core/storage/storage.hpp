#pragma once

// Accessors for the platform storage backends (ports, not concrete types).
// Phase 5 will fold these into the AppContext.

#include "interface.hpp"

namespace storage {

core::IStorage&   nvs();    // small blobs: keys, credentials, counters
core::IFileStore& files();  // files on the mounted LittleFS partition

} // namespace storage
