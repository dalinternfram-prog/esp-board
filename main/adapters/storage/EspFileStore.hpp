#pragma once

// Adapter: LittleFS-backed implementation of the core::IFileStore port.
// Reuses the existing FileManager mount/path helpers.

#include "core/ports.hpp"

#include <cstdio>
#include <string>

#include "file_manager.hpp"

namespace adapters {

class EspFileStore : public core::IFileStore {
public:
    bool init() override { return FileManager::init() == ESP_OK; }

    bool exists(const std::string& path) override {
        return FileManager::exists(path);
    }

    bool remove(const std::string& path) override {
        return FileManager::deleteFile(path);
    }

    bool readAll(const std::string& path, core::Bytes& out) override {
        const std::string full = FileManager::getFullPath(path);
        FILE* f = std::fopen(full.c_str(), "rb");
        if (!f) {
            return false;
        }
        std::fseek(f, 0, SEEK_END);
        const long size = std::ftell(f);
        std::rewind(f);
        if (size < 0) {
            std::fclose(f);
            return false;
        }
        out.resize(static_cast<size_t>(size));
        const size_t read = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
        std::fclose(f);
        return read == out.size();
    }

    bool writeAll(const std::string& path, const core::Bytes& data) override {
        const std::string full = FileManager::getFullPath(path);
        FILE* f = std::fopen(full.c_str(), "wb");
        if (!f) {
            return false;
        }
        const size_t written = data.empty() ? 0 : std::fwrite(data.data(), 1, data.size(), f);
        std::fclose(f);
        return written == data.size();
    }
};

} // namespace adapters
