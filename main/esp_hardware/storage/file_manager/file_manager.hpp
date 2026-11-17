#pragma once

// LittleFS mount + path resolution. File-level operations live in
// general/io/StdioFileStore (see adapters/storage/EspFileStore).

#include "esp_err.h"

#include <cstddef>
#include <string>

namespace FileManager {

// Mounts/registers the LittleFS partition on the VFS.
esp_err_t init(const std::string& base_path = "/littlefs",
               const std::string& partition_label = "my_files",
               size_t max_files = 5);

// Unmounts/unregisters the LittleFS partition.
esp_err_t deinit();

bool isMounted();

// Prefixes a relative path with the configured mount point.
std::string getFullPath(const std::string& filename);

} // namespace FileManager
