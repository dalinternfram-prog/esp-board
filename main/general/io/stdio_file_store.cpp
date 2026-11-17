#include "general/io/stdio_file_store.hpp"

#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

namespace general::io {

bool StdioFileStore::exists(const std::string& full_path) {
    struct stat st;
    return ::stat(full_path.c_str(), &st) == 0;
}

bool StdioFileStore::remove(const std::string& full_path) {
    return ::unlink(full_path.c_str()) == 0;
}

bool StdioFileStore::readAll(const std::string& full_path, std::vector<uint8_t>& out) {
    FILE* f = std::fopen(full_path.c_str(), "rb");
    if (f == nullptr) {
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::rewind(f);
    if (size < 0) {
        std::fclose(f);
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    const std::size_t read = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    std::fclose(f);
    return read == out.size();
}

bool StdioFileStore::writeAll(const std::string& full_path, const std::vector<uint8_t>& data) {
    FILE* f = std::fopen(full_path.c_str(), "wb");
    if (f == nullptr) {
        return false;
    }
    const std::size_t written = data.empty() ? 0 : std::fwrite(data.data(), 1, data.size(), f);
    std::fclose(f);
    return written == data.size();
}

} // namespace general::io
