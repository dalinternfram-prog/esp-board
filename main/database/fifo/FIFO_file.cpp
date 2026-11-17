// #include "FIFO_file.hpp"
// #include "esp_log.h"
// #include <algorithm>
// #include <cstring>

// namespace file {

// static const char* TAG = "FIFO";

// FIFO::FIFO(std::string fileName, size_t cacheCapacity)
//     : fileName_(std::move(fileName)),
//       file_(fileName_),
//       cacheCapacity_(cacheCapacity),
//       head_(HEADER_SIZE),
//       diskFileSize_(0) {
//     writeCache_.reserve(32 * 1024);
//     readCache_.reserve(16 * 1024);

//     initHeader();
// }

// FIFO::~FIFO() {
//     flush();
// }

// FIFO::FIFO(FIFO&& other) noexcept
//     : fileName_(std::move(other.fileName_)),
//       file_(std::move(other.file_)),
//       cacheCapacity_(other.cacheCapacity_),
//       writeCache_(std::move(other.writeCache_)),
//       writeCachePos_(other.writeCachePos_),
//       readCache_(std::move(other.readCache_)),
//       readCachePos_(other.readCachePos_),
//       head_(other.head_),
//       diskFileSize_(other.diskFileSize_) {
//     other.head_ = HEADER_SIZE;
//     other.diskFileSize_ = 0;
//     other.writeCachePos_ = 0;
//     other.readCachePos_ = 0;
// }

// FIFO& FIFO::operator=(FIFO&& other) noexcept {
//     if (this != &other) {
//         flush();

//         fileName_ = std::move(other.fileName_);
//         file_ = std::move(other.file_);
//         const_cast<size_t&>(cacheCapacity_) = other.cacheCapacity_;
//         writeCache_ = std::move(other.writeCache_);
//         writeCachePos_ = other.writeCachePos_;
//         readCache_ = std::move(other.readCache_);
//         readCachePos_ = other.readCachePos_;
//         head_ = other.head_;
//         diskFileSize_ = other.diskFileSize_;

//         other.head_ = HEADER_SIZE;
//         other.diskFileSize_ = 0;
//         other.writeCachePos_ = 0;
//         other.readCachePos_ = 0;
//     }
//     return *this;
// }

// void FIFO::initHeader() {
//     diskFileSize_ = static_cast<uint32_t>(file_.fileSize());

//     if (diskFileSize_ >= HEADER_SIZE) {
//         std::vector<uint8_t> hdr = file_.readFileBytes(0, HEADER_SIZE);
//         uint32_t magic = 0;
//         uint32_t loadedHead = 0;
//         std::memcpy(&magic, hdr.data(), sizeof(uint32_t));
//         std::memcpy(&loadedHead, hdr.data() + 4, sizeof(uint32_t));

//         if (magic == FIFO_MAGIC) {
//             head_ = loadedHead;
//             if (head_ < HEADER_SIZE || head_ > diskFileSize_) {
//                 head_ = HEADER_SIZE;
//                 writeHeadToDisk(true);
//             }

//             // If previously drained on last shutdown, clean up
//             if (head_ >= diskFileSize_ && diskFileSize_ > HEADER_SIZE) {
//                 file_.truncate(HEADER_SIZE);
//                 diskFileSize_ = HEADER_SIZE;
//                 head_ = HEADER_SIZE;
//                 writeHeadToDisk(true);
//             }

//             ESP_LOGI(TAG, "Initialized FIFO [%s], head: %u, diskSize: %u, RAM capacity: %u KB",
//                      fileName_.c_str(), (unsigned)head_, (unsigned)diskFileSize_, (unsigned)(cacheCapacity_ / 1024));
//             return;
//         }
//     }

//     // Format new header
//     uint8_t hdr[HEADER_SIZE] = {0};
//     uint32_t magic = FIFO_MAGIC;
//     uint32_t initHead = HEADER_SIZE;
//     std::memcpy(hdr, &magic, sizeof(uint32_t));
//     std::memcpy(hdr + 4, &initHead, sizeof(uint32_t));

//     file_.writeFile(std::vector<uint8_t>(hdr, hdr + HEADER_SIZE), 0);
//     file_.flush();
//     head_ = HEADER_SIZE;
//     diskFileSize_ = HEADER_SIZE;
//     ESP_LOGI(TAG, "Formatted new FIFO file [%s]", fileName_.c_str());
// }

// void FIFO::writeHeadToDisk(bool forceSync) {
//     uint8_t buf[sizeof(uint32_t)];
//     std::memcpy(buf, &head_, sizeof(uint32_t));
//     file_.writeFile(std::vector<uint8_t>(buf, buf + sizeof(uint32_t)), 4);
//     if (forceSync) {
//         file_.flush();
//     }
// }

// // ----------------------------------------------------------------------------
// // Commit unwritten RAM data to physical disk
// // ----------------------------------------------------------------------------
// void FIFO::commitWriteCacheToDisk() {
//     // Only write remaining unpopped bytes from writeCache_
//     if (writeCachePos_ < writeCache_.size()) {
//         size_t bytesToWrite = writeCache_.size() - writeCachePos_;
//         std::vector<uint8_t> slice(writeCache_.begin() + writeCachePos_, writeCache_.end());
//         file_.writeFile(slice, -1);
//         diskFileSize_ += static_cast<uint32_t>(bytesToWrite);
//     }
//     writeCache_.clear();
//     writeCachePos_ = 0;
// }

// void FIFO::push_back(std::vector<uint8_t> data) {
//     if (data.empty()) return;

//     uint32_t len = static_cast<uint32_t>(data.size());
//     size_t packetSize = sizeof(uint32_t) + len;

//     // Check if adding this packet exceeds configured RAM limit
//     if ((writeCache_.size() - writeCachePos_) + packetSize > cacheCapacity_) {
//         // Spill RAM buffer to flash
//         commitWriteCacheToDisk();
//     }

//     // Serialize packet: [length (4B)] + [data]
//     uint8_t lenBuf[sizeof(uint32_t)];
//     std::memcpy(lenBuf, &len, sizeof(uint32_t));
//     writeCache_.insert(writeCache_.end(), lenBuf, lenBuf + sizeof(uint32_t));
//     writeCache_.insert(writeCache_.end(), data.begin(), data.end());
// }

// void FIFO::flush() {
//     commitWriteCacheToDisk();
//     writeHeadToDisk(true);
// }

// // ----------------------------------------------------------------------------
// // Pop(): Guaranteed Strict FIFO Ordering (Disk first, then RAM)
// // ----------------------------------------------------------------------------
// std::vector<uint8_t> FIFO::Pop() {
//     // =========================================================================
//     // CASE 1: There is older data on disk. Must drain disk first!
//     // =========================================================================
//     if (head_ < diskFileSize_) {
//         // Ensure read cache has at least 4 bytes for length header
//         if (readCachePos_ + sizeof(uint32_t) > readCache_.size()) {
//             if (!refillReadCache()) {
//                 // If refill fails, disk read error
//                 return {};
//             }
//         }

//         uint32_t len = 0;
//         std::memcpy(&len, readCache_.data() + readCachePos_, sizeof(uint32_t));
//         size_t totalBytes = sizeof(uint32_t) + len;

//         // Fast path: packet is fully in readCache_
//         if (readCachePos_ + totalBytes <= readCache_.size()) {
//             size_t start = readCachePos_ + sizeof(uint32_t);
//             std::vector<uint8_t> pkt(readCache_.begin() + start, readCache_.begin() + start + len);

//             readCachePos_ += totalBytes;
//             head_ += totalBytes;

//             // Check if disk has caught up with all written disk bytes
//             if (head_ >= diskFileSize_) {
//                 readCache_.clear();
//                 readCachePos_ = 0;

//                 // CRITICAL FIX: Look at writeCache_ before truncating!
//                 if (writeCachePos_ >= writeCache_.size()) {
//                     // Both disk AND writeCache are empty! Safe to reset file completely!
//                     file_.truncate(HEADER_SIZE);
//                     head_ = HEADER_SIZE;
//                     diskFileSize_ = HEADER_SIZE;
//                     writeHeadToDisk(false);
//                     writeCache_.clear();
//                     writeCachePos_ = 0;
//                 } else {
//                     // Disk is caught up, but there is newer data in writeCache_!
//                     // Do NOT truncate. The subsequent Pop() calls will seamlessly
//                     // drain from writeCache_ below without touching disk!
//                 }
//             }

//             return pkt;
//         }

//         // Cache boundary: reload next aligned chunk
//         refillReadCache();
//         return Pop();
//     }

//     // =========================================================================
//     // CASE 2: Disk is empty (or caught up). Pop directly from writeCache_ in RAM!
//     // =========================================================================
//     if (writeCachePos_ < writeCache_.size()) {
//         if (writeCachePos_ + sizeof(uint32_t) > writeCache_.size()) {
//             // Corrupt trailing bytes in RAM, reset
//             writeCache_.clear();
//             writeCachePos_ = 0;
//             return {};
//         }

//         uint32_t len = 0;
//         std::memcpy(&len, writeCache_.data() + writeCachePos_, sizeof(uint32_t));
//         size_t totalBytes = sizeof(uint32_t) + len;

//         if (writeCachePos_ + totalBytes > writeCache_.size()) {
//             writeCache_.clear();
//             writeCachePos_ = 0;
//             return {};
//         }

//         size_t start = writeCachePos_ + sizeof(uint32_t);
//         std::vector<uint8_t> pkt(writeCache_.begin() + start, writeCache_.begin() + start + len);

//         writeCachePos_ += totalBytes;

//         // If writeCache is completely consumed, reset vectors without memory allocations
//         if (writeCachePos_ >= writeCache_.size()) {
//             writeCache_.clear();
//             writeCachePos_ = 0;

//             // If physical disk had leftover space, truncate now
//             if (head_ >= diskFileSize_ && diskFileSize_ > HEADER_SIZE) {
//                 file_.truncate(HEADER_SIZE);
//                 head_ = HEADER_SIZE;
//                 diskFileSize_ = HEADER_SIZE;
//                 writeHeadToDisk(false);
//             }
//         }

//         return pkt;
//     }

//     return {};
// }

// bool FIFO::empty() {
//     bool diskEmpty = (head_ >= diskFileSize_);
//     bool ramEmpty  = (writeCachePos_ >= writeCache_.size());
//     return (diskEmpty && ramEmpty);
// }

// bool FIFO::refillReadCache() {
//     if (head_ >= diskFileSize_) {
//         readCache_.clear();
//         readCachePos_ = 0;
//         return false;
//     }

//     size_t bytesToRead = std::min(static_cast<size_t>(16 * 1024), static_cast<size_t>(diskFileSize_ - head_));
//     readCache_ = file_.readFileBytes(head_, bytesToRead);
//     readCachePos_ = 0;
//     return !readCache_.empty();
// }

// } // namespace file

















#include "FIFO_file.hpp"
#include "esp_log.h"
#include <algorithm>
#include <cstring>

namespace file {

static const char* TAG = "FIFO";

FIFO::FIFO(std::string fileName, size_t cacheCapacity)
    : fileName_(std::move(fileName)),
      file_(fileName_),
      cacheCapacity_(cacheCapacity),
      head_(HEADER_SIZE),
      diskFileSize_(0) {
    writeCache_.reserve(32 * 1024);
    readCache_.reserve(16 * 1024);

    initHeader();
}

FIFO::~FIFO() {
    flush();
}

FIFO::FIFO(FIFO&& other) noexcept
    : fileName_(std::move(other.fileName_)),
      file_(std::move(other.file_)),
      cacheCapacity_(other.cacheCapacity_),
      writeCache_(std::move(other.writeCache_)),
      writeCachePos_(other.writeCachePos_),
      readCache_(std::move(other.readCache_)),
      readCachePos_(other.readCachePos_),
      head_(other.head_),
      diskFileSize_(other.diskFileSize_) {
    other.head_ = HEADER_SIZE;
    other.diskFileSize_ = 0;
    other.writeCachePos_ = 0;
    other.readCachePos_ = 0;
}

FIFO& FIFO::operator=(FIFO&& other) noexcept {
    if (this != &other) {
        flush();

        fileName_ = std::move(other.fileName_);
        file_ = std::move(other.file_);
        const_cast<size_t&>(cacheCapacity_) = other.cacheCapacity_;
        writeCache_ = std::move(other.writeCache_);
        writeCachePos_ = other.writeCachePos_;
        readCache_ = std::move(other.readCache_);
        readCachePos_ = other.readCachePos_;
        head_ = other.head_;
        diskFileSize_ = other.diskFileSize_;

        other.head_ = HEADER_SIZE;
        other.diskFileSize_ = 0;
        other.writeCachePos_ = 0;
        other.readCachePos_ = 0;
    }
    return *this;
}

void FIFO::initHeader() {
    diskFileSize_ = static_cast<uint32_t>(file_.fileSize());

    if (diskFileSize_ >= HEADER_SIZE) {
        std::vector<uint8_t> hdr = file_.readFileBytes(0, HEADER_SIZE);
        uint32_t magic = 0;
        uint32_t loadedHead = 0;
        std::memcpy(&magic, hdr.data(), sizeof(uint32_t));
        std::memcpy(&loadedHead, hdr.data() + 4, sizeof(uint32_t));

        if (magic == FIFO_MAGIC) {
            head_ = loadedHead;
            if (head_ < HEADER_SIZE || head_ > diskFileSize_) {
                head_ = HEADER_SIZE;
                writeHeadToDisk(true);
            }

            // If head caught up to disk size on boot, reset cleanly
            if (head_ >= diskFileSize_ && diskFileSize_ > HEADER_SIZE) {
                file_.truncate(HEADER_SIZE);
                diskFileSize_ = HEADER_SIZE;
                head_ = HEADER_SIZE;
                writeHeadToDisk(true);
            }

            ESP_LOGI(TAG, "Initialized FIFO [%s], head: %u, diskSize: %u, RAM capacity: %u KB",
                     fileName_.c_str(), (unsigned)head_, (unsigned)diskFileSize_, (unsigned)(cacheCapacity_ / 1024));
            return;
        }
    }

    // Format new header
    uint8_t hdr[HEADER_SIZE] = {0};
    uint32_t magic = FIFO_MAGIC;
    uint32_t initHead = HEADER_SIZE;
    std::memcpy(hdr, &magic, sizeof(uint32_t));
    std::memcpy(hdr + 4, &initHead, sizeof(uint32_t));

    file_.writeFile(std::vector<uint8_t>(hdr, hdr + HEADER_SIZE), 0);
    file_.flush();
    head_ = HEADER_SIZE;
    diskFileSize_ = HEADER_SIZE;
    ESP_LOGI(TAG, "Formatted new FIFO file [%s]", fileName_.c_str());
}

void FIFO::writeHeadToDisk(bool forceSync) {
    uint8_t buf[sizeof(uint32_t)];
    std::memcpy(buf, &head_, sizeof(uint32_t));
    file_.writeFile(std::vector<uint8_t>(buf, buf + sizeof(uint32_t)), 4);
    if (forceSync) {
        file_.flush();
    }
}

void FIFO::commitWriteCacheToDisk() {
    if (writeCachePos_ < writeCache_.size()) {
        size_t bytesToWrite = writeCache_.size() - writeCachePos_;
        std::vector<uint8_t> slice(writeCache_.begin() + writeCachePos_, writeCache_.end());
        file_.writeFile(slice, -1);
        diskFileSize_ += static_cast<uint32_t>(bytesToWrite);
    }
    writeCache_.clear();
    writeCachePos_ = 0;
}

void FIFO::push_back(std::vector<uint8_t> data) {
    if (data.empty()) return;

    uint32_t len = static_cast<uint32_t>(data.size());
    size_t packetSize = sizeof(uint32_t) + len;

    // Check if adding this packet exceeds configured RAM limit
    if ((writeCache_.size() - writeCachePos_) + packetSize > cacheCapacity_) {
        commitWriteCacheToDisk();
    }

    // Append to writeCache_: [length (4B)] + [payload]
    uint8_t lenBuf[sizeof(uint32_t)];
    std::memcpy(lenBuf, &len, sizeof(uint32_t));
    writeCache_.insert(writeCache_.end(), lenBuf, lenBuf + sizeof(uint32_t));
    writeCache_.insert(writeCache_.end(), data.begin(), data.end());
}

void FIFO::flush() {
    commitWriteCacheToDisk();
    writeHeadToDisk(true);
}

// ----------------------------------------------------------------------------
// Pop: Fixed Alignment & Buffer Safety
// ----------------------------------------------------------------------------
std::vector<uint8_t> FIFO::Pop() {
    // =========================================================================
    // CASE 1: Data exists on physical disk (head_ < diskFileSize_)
    // =========================================================================
    if (head_ < diskFileSize_) {
        // Ensure read cache has at least the 4-byte length header
        if (readCachePos_ + sizeof(uint32_t) > readCache_.size()) {
            if (!refillReadCache()) {
                return {};
            }
        }

        uint32_t len = 0;
        std::memcpy(&len, readCache_.data() + readCachePos_, sizeof(uint32_t));
        size_t totalBytes = sizeof(uint32_t) + len;

        // Path A: The complete packet is within readCache_
        if (readCachePos_ + totalBytes <= readCache_.size()) {
            size_t start = readCachePos_ + sizeof(uint32_t);
            std::vector<uint8_t> pkt(readCache_.begin() + start, readCache_.begin() + start + len);

            readCachePos_ += totalBytes;
            head_ += totalBytes;

            // Check if disk backlog is completely drained
            if (head_ >= diskFileSize_) {
                readCache_.clear();
                readCachePos_ = 0;

                // Only truncate file if writeCache in RAM is ALSO empty
                if (writeCachePos_ >= writeCache_.size()) {
                    file_.truncate(HEADER_SIZE);
                    head_ = HEADER_SIZE;
                    diskFileSize_ = HEADER_SIZE;
                    writeHeadToDisk(false);
                    writeCache_.clear();
                    writeCachePos_ = 0;
                }
            }

            return pkt;
        }

        // Path B: Packet crosses the readCache_ boundary!
        // Instead of blind recursion, read the exact packet directly starting from head_
        std::vector<uint8_t> directBytes = file_.readFileBytes(head_ + sizeof(uint32_t), len);
        head_ += totalBytes;
        
        // Invalidate read cache so next pop reloads aligned from new head_
        readCache_.clear();
        readCachePos_ = 0;

        if (head_ >= diskFileSize_) {
            if (writeCachePos_ >= writeCache_.size()) {
                file_.truncate(HEADER_SIZE);
                head_ = HEADER_SIZE;
                diskFileSize_ = HEADER_SIZE;
                writeHeadToDisk(false);
                writeCache_.clear();
                writeCachePos_ = 0;
            }
        }

        return directBytes;
    }

    // =========================================================================
    // CASE 2: Disk is empty. Pop directly from RAM writeCache_!
    // =========================================================================
    size_t ramRemaining = writeCache_.size() - writeCachePos_;
    if (ramRemaining >= sizeof(uint32_t)) {
        uint32_t len = 0;
        std::memcpy(&len, writeCache_.data() + writeCachePos_, sizeof(uint32_t));
        size_t totalBytes = sizeof(uint32_t) + len;

        if (totalBytes <= ramRemaining) {
            size_t start = writeCachePos_ + sizeof(uint32_t);
            std::vector<uint8_t> pkt(writeCache_.begin() + start, writeCache_.begin() + start + len);

            writeCachePos_ += totalBytes;

            // If writeCache is completely consumed, clear to reclaim memory
            if (writeCachePos_ >= writeCache_.size()) {
                writeCache_.clear();
                writeCachePos_ = 0;

                if (head_ >= diskFileSize_ && diskFileSize_ > HEADER_SIZE) {
                    file_.truncate(HEADER_SIZE);
                    head_ = HEADER_SIZE;
                    diskFileSize_ = HEADER_SIZE;
                    writeHeadToDisk(false);
                }
            }

            return pkt;
        }
    }

    return {};
}

bool FIFO::empty() {
    bool diskEmpty = (head_ >= diskFileSize_);
    bool ramEmpty  = (writeCachePos_ >= writeCache_.size());
    return (diskEmpty && ramEmpty);
}

bool FIFO::refillReadCache() {
    if (head_ >= diskFileSize_) {
        readCache_.clear();
        readCachePos_ = 0;
        return false;
    }

    size_t bytesToRead = std::min(static_cast<size_t>(16 * 1024), static_cast<size_t>(diskFileSize_ - head_));
    readCache_ = file_.readFileBytes(head_, bytesToRead);
    readCachePos_ = 0;
    return !readCache_.empty();
}

} // namespace file