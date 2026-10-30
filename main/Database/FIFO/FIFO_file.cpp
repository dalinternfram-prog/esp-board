#include "FIFO_file.hpp" // Class declaration header
#include "esp_log.h"      // ESP-IDF logging library
#include <algorithm>      // std::min and std::max
#include <cstring>        // std::memcpy

namespace file { // Opens file namespace implementation block

static const char* TAG = "FIFO"; // Logging identifier for console messages

FIFO::FIFO(std::string fileName) // Constructor definition
    : fileName_(std::move(fileName)), // Moves input filename into member variable
      file_(fileName_), // Initializes SingleFile instance with target filename
      head_(HEADER_SIZE) { // Defaults head to point immediately after the header
    cache_.reserve(CACHE_CAPACITY); // Pre-allocates memory for the cache to prevent reallocations
    initHeader(); // Reads existing head from file header or formats new header on boot
    begin_push_mode(); // Starts in push mode by default
} // Closes constructor definition

FIFO::~FIFO() { // Destructor definition
    if (pushMode_) { // If destroying while in push mode
        flush(); // Flush any remaining cached data packets
    } // Closes push check
    writeHeadToDisk(true); // Persist final head position to disk with full stream flush
} // Closes destructor definition

FIFO::FIFO(FIFO&& other) noexcept // Move constructor definition
    : fileName_(std::move(other.fileName_)), // Adopts filename
      file_(std::move(other.file_)), // Adopts SingleFile handle
      cache_(std::move(other.cache_)), // Adopts cache buffer
      cachePos_(other.cachePos_), // Copies cache position
      head_(other.head_), // Copies head pointer
      pushMode_(other.pushMode_), // Copies push mode flag
      popMode_(other.popMode_) { // Copies pop mode flag
    other.cachePos_ = 0; // Clears other's cursor
    other.head_ = HEADER_SIZE; // Resets other's head pointer
    other.pushMode_ = false; // Resets other's push flag
    other.popMode_ = false; // Resets other's pop flag
} // Closes move constructor definition

FIFO& FIFO::operator=(FIFO&& other) noexcept { // Move assignment operator definition
    if (this != &other) { // Protects against self-assignment
        if (pushMode_) { // Check for uncommitted writes
            flush(); // Flush writes before releasing resources
        } // Closes flush check
        writeHeadToDisk(true); // Save current head state

        fileName_ = std::move(other.fileName_); // Transfer filename
        file_ = std::move(other.file_); // Transfer SingleFile handle
        cache_ = std::move(other.cache_); // Transfer cache
        cachePos_ = other.cachePos_; // Transfer cache cursor
        head_ = other.head_; // Transfer head offset
        pushMode_ = other.pushMode_; // Transfer push flag
        popMode_ = other.popMode_; // Transfer pop flag

        other.cachePos_ = 0; // Invalidate other's cursor
        other.head_ = HEADER_SIZE; // Reset other's head
        other.pushMode_ = false; // Reset other's push flag
        other.popMode_ = false; // Reset other's pop flag
    } // Closes self-assignment check
    return *this; // Return reference to self
} // Closes move assignment definition

void FIFO::initHeader() { // Restores or initializes header on boot
    size_t sz = file_.fileSize(); // Query current file length on flash

    if (sz >= HEADER_SIZE) { // File already contains at least the header
        std::vector<uint8_t> hdr = file_.readFileBytes(0, HEADER_SIZE); // Read 8-byte header
        uint32_t magic = 0; // Temporary magic holder
        uint32_t loadedHead = 0; // Temporary head holder
        std::memcpy(&magic, hdr.data(), sizeof(uint32_t)); // Extract 4-byte magic
        std::memcpy(&loadedHead, hdr.data() + 4, sizeof(uint32_t)); // Extract 4-byte head offset

        if (magic == FIFO_MAGIC) { // Validate magic identifier
            head_ = loadedHead; // Restore persistent head position from previous boot
            if (head_ < HEADER_SIZE || head_ > sz) { // Sanity check for corruption
                head_ = HEADER_SIZE; // Reset to start if corrupted
                writeHeadToDisk(true); // Persist corrected head
            } // Closes corruption check
            ESP_LOGI(TAG, "Loaded existing FIFO [%s], head at byte %u", fileName_.c_str(), (unsigned)head_); // Log boot load
            return; // Successfully restored from disk
        } // Closes magic check
    } // Closes existing file check

    // File is empty or not yet configured as FIFO: initialize new header
    uint8_t hdr[HEADER_SIZE] = {0}; // Zero-initialized header buffer
    uint32_t magic = FIFO_MAGIC; // Magic value
    uint32_t initHead = HEADER_SIZE; // Initial head points immediately after header
    std::memcpy(hdr, &magic, sizeof(uint32_t)); // Copy magic to offset 0
    std::memcpy(hdr + 4, &initHead, sizeof(uint32_t)); // Copy initial head (8) to offset 4

    file_.writeFile(std::vector<uint8_t>(hdr, hdr + HEADER_SIZE), 0); // Write 8-byte header at file offset 0
    file_.flush(); // Ensure header is committed to flash
    head_ = HEADER_SIZE; // Set in-memory head to point after header
    ESP_LOGI(TAG, "Configured new FIFO file [%s]", fileName_.c_str()); // Log initial configuration
} // Closes initHeader definition

void FIFO::writeHeadToDisk(bool forceFlush) { // Saves head_ to offset 4 of the file
    uint8_t buf[sizeof(uint32_t)]; // Stack buffer for 4 bytes
    std::memcpy(buf, &head_, sizeof(uint32_t)); // Serialize head_ in little-endian format
    file_.writeFile(std::vector<uint8_t>(buf, buf + sizeof(uint32_t)), 4); // Overwrite offset 4 in file
    if (forceFlush) { // If immediate physical sync requested
        file_.flush(); // Force commit to SPIFFS flash
    } // Closes forceFlush check
} // Closes writeHeadToDisk definition

void FIFO::begin_push_mode() { // Switches to write/push mode
    if (pushMode_) return; // Already in push mode; no-op

    if (popMode_) { // Transitioning from pop mode
        cache_.clear(); // Discard pop read-ahead cache
        cachePos_ = 0; // Reset read cursor
        popMode_ = false; // Deactivate pop mode
        maybeCompact(); // Only shift if the gap is truly large
    } // Closes popMode transition check

    pushMode_ = true; // Activate push mode
    cache_.clear(); // Clear cache for write buffering
    cache_.reserve(CACHE_CAPACITY); // Ensure buffer has reserved capacity
} // Closes begin_push_mode definition

void FIFO::begin_pop_mode() { // Switches to read/pop mode
    if (popMode_) return; // Already in pop mode; no-op

    if (pushMode_) { // Transitioning from push mode
        flush(); // Commit pending write cache to flash
        pushMode_ = false; // Deactivate push mode
    } // Closes pushMode transition check

    popMode_ = true; // Activate pop mode
    cache_.clear(); // Clear write buffer residue
    cachePos_ = 0; // Reset read cursor
    refillPopCache(); // Prime read-ahead buffer from current head_ position
} // Closes begin_pop_mode definition

void FIFO::push_back(std::vector<uint8_t> data) { // Enqueues a packet
    if (!pushMode_) { // Auto-switch to push mode if needed
        begin_push_mode(); // Switch mode
    } // Closes auto-switch check

    uint32_t len = static_cast<uint32_t>(data.size()); // Payload length
    size_t totalBytesNeeded = sizeof(uint32_t) + len; // 4-byte header + payload length

    if (cache_.size() + totalBytesNeeded > CACHE_CAPACITY) { // If cache cannot fit packet
        flush(); // Commit accumulated cache to flash
    } // Closes capacity check

    if (totalBytesNeeded > CACHE_CAPACITY) { // Oversized packet larger than whole cache
        uint8_t header[sizeof(uint32_t)]; // Header buffer
        std::memcpy(header, &len, sizeof(uint32_t)); // Serialize length
        file_.writeFile(std::vector<uint8_t>(header, header + sizeof(uint32_t)), -1); // Append header
        file_.writeFile(data, -1); // Append payload directly
        file_.flush(); // Ensure oversized write is committed
        return; // Done
    } // Closes oversized packet check

    // Serialize 4-byte length prefix directly into cache_
    uint8_t lenBuf[sizeof(uint32_t)]; // Stack buffer for length
    std::memcpy(lenBuf, &len, sizeof(uint32_t)); // Copy little-endian length
    cache_.insert(cache_.end(), lenBuf, lenBuf + sizeof(uint32_t)); // Append header bytes to cache

    // Append payload bytes into cache_
    cache_.insert(cache_.end(), data.begin(), data.end()); // Copy payload into cache
} // Closes push_back definition

void FIFO::flush() { // Commits write cache and persists head
    if (pushMode_ && !cache_.empty()) { // If there are pending unwritten packets
        file_.writeFile(cache_, -1); // Append cache to end of file on disk
        cache_.clear(); // Empty cache
    } // Closes write check
    writeHeadToDisk(true); // Persist current head pointer to disk header and flush stream
} // Closes flush definition

std::vector<uint8_t> FIFO::Pop() { // Dequeues the next packet
    if (!popMode_) { // Auto-switch to pop mode if needed
        begin_pop_mode(); // Switch mode
    } // Closes auto-switch check

    // Check if cache contains at least 4 bytes for the length header
    if (cachePos_ + sizeof(uint32_t) > cache_.size()) { // Cache has fewer than 4 bytes
        if (!refillPopCache()) { // Fetch next block from disk at head_
            return {}; // Return empty if EOF reached
        } // Closes refill check
        if (cachePos_ + sizeof(uint32_t) > cache_.size()) { // Still insufficient bytes after refill
            return {}; // Corrupted or truncated packet; abort
        } // Closes secondary length check
    } // Closes header presence check

    // Decode 4-byte length header
    uint32_t len = 0; // Length variable
    std::memcpy(&len, cache_.data() + cachePos_, sizeof(uint32_t)); // Read length from cache
    size_t totalPacketBytes = sizeof(uint32_t) + len; // Calculate full frame size

    // Fast path: entire packet resides in cache_
    if (cachePos_ + totalPacketBytes <= cache_.size()) { // Packet fits inside buffer
        size_t dataStart = cachePos_ + sizeof(uint32_t); // Calculate payload start index
        size_t dataEnd = dataStart + len; // Calculate payload end index

        std::vector<uint8_t> packet(cache_.begin() + dataStart, cache_.begin() + dataEnd); // Extract payload

        cachePos_ += totalPacketBytes; // Advance cache read cursor
        head_ += totalPacketBytes; // Advance disk head pointer
        writeHeadToDisk(false); // Update head on disk (buffered, no immediate flush)

        if (cachePos_ == cache_.size()) { // Cache fully exhausted
            cache_.clear(); // Clear empty vector
            cachePos_ = 0; // Reset cursor to 0
        } // Closes cache exhaustion check

        if (head_ >= file_.fileSize()) { // All items popped: entire queue is empty!
            file_.truncate(HEADER_SIZE); // Truncate file immediately back to header (Zero shifting!)
            head_ = HEADER_SIZE; // Reset head to start after header
            writeHeadToDisk(true); // Persist reset head to disk
            cache_.clear(); // Clear cache
            cachePos_ = 0; // Reset cursor
        } // Closes empty queue check

        return packet; // Return extracted payload
    } // Closes fast path

    // Medium path: packet spans across cache chunks but fits in fresh cache
    if (totalPacketBytes <= CACHE_CAPACITY) { // Fits within fresh buffer
        refillPopCache(); // Reload buffer starting exactly from head_
        return Pop(); // Pop from newly aligned cache
    } // Closes medium packet check

    // Oversized packet path: read directly from disk
    std::vector<uint8_t> packet = file_.readFileBytes(head_ + sizeof(uint32_t), len); // Direct read
    head_ += totalPacketBytes; // Advance head pointer
    writeHeadToDisk(false); // Record updated head
    cache_.clear(); // Invalidate cache
    cachePos_ = 0; // Reset cursor

    if (head_ >= file_.fileSize()) { // All items popped
        file_.truncate(HEADER_SIZE); // Truncate to header (Zero shifting!)
        head_ = HEADER_SIZE; // Reset head
        writeHeadToDisk(true); // Persist reset
    } // Closes empty queue check

    return packet; // Return oversized payload
} // Closes Pop definition

bool FIFO::empty() { // Checks if queue has pending items
    if (pushMode_) { // In push mode
        return cache_.empty() && (file_.fileSize() <= HEADER_SIZE); // Check cache and disk length
    } // Closes pushMode check

    // In pop mode: empty if cache is exhausted and head has reached end of file
    return (cachePos_ >= cache_.size()) && (head_ >= file_.fileSize()); // Evaluates unread content
} // Closes empty definition

void FIFO::maybeCompact() { // Evaluates gap size and only shifts if really necessary
    size_t sz = file_.fileSize(); // Query physical file size
    if (head_ >= sz) { // Queue is completely empty
        file_.truncate(HEADER_SIZE); // Instant wipe down to header (Zero shift wear!)
        head_ = HEADER_SIZE; // Reset head
        writeHeadToDisk(true); // Persist reset head
        return; // Done
    } // Closes empty check

    size_t deadGap = head_ - HEADER_SIZE; // Calculate wasted dead space at front of file
    // ONLY shift if dead gap is really big OR file size is too big
    if (deadGap >= GAP_THRESHOLD || sz >= MAX_FILE_THRESHOLD) { // Threshold evaluation
        compactDisk(); // Perform flash shift
    } // Closes threshold check
} // Closes maybeCompact definition

void FIFO::compactDisk() { // Performs physical flash byte shift
    size_t sz = file_.fileSize(); // Query file size
    if (head_ <= HEADER_SIZE || head_ >= sz) return; // Nothing to shift or empty

    size_t shiftDistance = head_ - HEADER_SIZE; // Calculate distance to slide unread region forward
    ESP_LOGI(TAG, "Compacting FIFO [%s]: shifting %u bytes left", fileName_.c_str(), (unsigned)shiftDistance); // Log compaction

    file_.shiftBytes(shiftDistance, true, head_, sz); // Slide unread region [head_ .. sz] to HEADER_SIZE
    file_.truncate(sz - shiftDistance); // Truncate duplicate dead bytes from tail
    head_ = HEADER_SIZE; // Reset head pointer to start immediately after header
    writeHeadToDisk(true); // Persist updated head to disk
} // Closes compactDisk definition

bool FIFO::refillPopCache() { // Loads next read-ahead block into cache_
    size_t sz = file_.fileSize(); // Query total file length
    if (head_ >= sz) { // Head reached EOF
        cache_.clear(); // Empty cache
        cachePos_ = 0; // Reset cursor
        return false; // Report EOF
    } // Closes EOF check

    cache_ = file_.readFileBytes(head_, CACHE_CAPACITY); // Read up to CACHE_CAPACITY bytes starting at head_
    cachePos_ = 0; // Reset cursor to beginning of loaded block
    return !cache_.empty(); // Return true if data was fetched
} // Closes refillPopCache definition

} // namespace file // Closes namespace