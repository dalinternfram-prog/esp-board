#pragma once

// Adapter: core::IPlaylist over the SD card directory listing.
// Portable listing logic lives in general/fs/DirectoryLister.

#include "interface.hpp"

#include "general/fs/directory_lister.hpp"

namespace adapters {

class EspSdPlaylist : public core::IPlaylist {
public:
    std::vector<std::string> scan(const std::string& directory,
                                  const std::string& extension) override {
        return general::fs::listFiles(directory, extension, 0);
    }
};

} // namespace adapters
