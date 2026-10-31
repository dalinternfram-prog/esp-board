#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "file_manager.hpp"
#include "single_file.hpp"

namespace file {

class FIFO {
public:
    explicit FIFO(std::string fileName, size_t cacheCapacity = 64 * 1024);
    ~FIFO();

    // Disable copy
    FIFO(const FIFO&) = delete;
    FIFO& operator=(const FIFO&) = delete;

    // Move semantics
    FIFO(FIFO&& other) noexcept;
    FIFO& operator=(FIFO&& other) noexcept;

    // Core APIs
    void push_back(std::vector<uint8_t> data);
    std::vector<uint8_t> Pop();
    void flush();
    bool empty();

 

private:
    std::string fileName_;
    FileManager::SingleFile file_;
    const size_t cacheCapacity_;

    // In-memory write buffer
    std::vector<uint8_t> writeCache_;
    size_t writeCachePos_ = 0; // Read cursor when popping directly from writeCache_

    // In-memory read buffer (for prefetching from disk)
    std::vector<uint8_t> readCache_;
    size_t readCachePos_ = 0;

    uint32_t head_ = 0;         // Disk read offset
    uint32_t diskFileSize_ = 0; // Physical file size on flash

    static constexpr uint32_t FIFO_MAGIC = 0x4F464946;
    static constexpr size_t HEADER_SIZE  = 8;

    void initHeader();
    void writeHeadToDisk(bool forceSync = false);
    bool refillReadCache();
    void commitWriteCacheToDisk(); // Spills writeCache_ to disk safely
};

} // namespace file