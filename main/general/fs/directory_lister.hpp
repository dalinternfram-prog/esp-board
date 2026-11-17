#pragma once

// Portable directory listing with an extension filter (POSIX dirent).
// Works on ESP-IDF VFS and on a POSIX host. No logging / no fixed buffers.

#include <cstddef>
#include <string>
#include <vector>

namespace general::fs {

// Returns matching file names (not full paths). max_files == 0 means no limit.
std::vector<std::string> listFiles(const std::string& directory,
                                   const std::string& extension,
                                   std::size_t max_files);

} // namespace general::fs
