#pragma once

// Adapter: LittleFS-backed implementation of the core::IFileStore port.
// Mount/path resolution stays here; portable stdio logic lives in
// general/io/StdioFileStore.

#include "interface.hpp"

#include <string>

#include "file_manager.hpp"
#include "general/io/stdio_file_store.hpp"

namespace adapters {

class EspFileStore : public core::IFileStore {
public:
    bool init() override { return FileManager::init() == ESP_OK; }

    bool exists(const std::string& path) override {
        return general::io::StdioFileStore::exists(FileManager::getFullPath(path));
    }

    bool remove(const std::string& path) override {
        return general::io::StdioFileStore::remove(FileManager::getFullPath(path));
    }

    bool readAll(const std::string& path, core::Bytes& out) override {
        return general::io::StdioFileStore::readAll(FileManager::getFullPath(path), out);
    }

    bool writeAll(const std::string& path, const core::Bytes& data) override {
        return general::io::StdioFileStore::writeAll(FileManager::getFullPath(path), data);
    }
};

} // namespace adapters

