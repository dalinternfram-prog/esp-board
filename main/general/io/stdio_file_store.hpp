#pragma once

// Portable stdio-backed file operations. Callers supply absolute paths; mount
// points and filesystem drivers stay in the hardware layer.

#include <cstdint>
#include <string>
#include <vector>

namespace general::io {

class StdioFileStore {
public:
    static bool exists(const std::string& full_path);
    static bool remove(const std::string& full_path);
    static bool readAll(const std::string& full_path, std::vector<uint8_t>& out);
    static bool writeAll(const std::string& full_path, const std::vector<uint8_t>& data);
};

} // namespace general::io
