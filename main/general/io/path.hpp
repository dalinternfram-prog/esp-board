#pragma once

// Portable path joining (VFS-style, no std::filesystem dependency).

#include <string>

namespace general::io {

inline std::string join(const std::string& base, const std::string& name) {
    if (name.empty()) {
        return base;
    }
    if (name.front() == '/') {
        return base + name;
    }
    if (base.empty()) {
        return name;
    }
    if (base.back() == '/') {
        return base + name;
    }
    return base + "/" + name;
}

} // namespace general::io
