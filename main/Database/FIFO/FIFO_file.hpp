#pragma once // Prevents double inclusion of this header during compilation

#include <cstddef>          // Standard size definitions like size_t
#include <cstdint>          // Standard integer types such as uint8_t and uint32_t
#include <string>           // Standard string class for file path representations
#include <vector>           // Standard vector container for packet byte payloads

#include "file_manager.hpp" // Level 1 filesystem base helpers
#include "single_file.hpp"  // Level 2 SingleFile class for persistent file handle operations

namespace file { // Opens dedicated file namespace

class FIFO { // Implements persistent disk-backed FIFO queue with on-disk head tracking
public:
    // Binds instance to file, initializes/validates FIFO header, and restores head_ on boot
    explicit FIFO(std::string fileName); // Constructs FIFO and loads persistent head from disk

    // Destructor: flushes pending writes and ensures current head position is persisted
    ~FIFO(); // Safely closes handle and updates disk header upon destruction

    // Disable copy semantics to prevent two instances manipulating the same underlying file
    FIFO(const FIFO&) = delete; // Deletes copy constructor
    FIFO& operator=(const FIFO&) = delete; // Deletes copy assignment operator

    // Enable move semantics to allow transferring FIFO ownership safely
    FIFO(FIFO&& other) noexcept; // Move constructor transfers buffers, handle, and offsets
    FIFO& operator=(FIFO&& other) noexcept; // Move assignment operator transfers ownership

    // ------------------------------------------------------------------------
    // Mode Switching APIs
    // ------------------------------------------------------------------------

    // Prepares FIFO for appending packets, checking if dead gap requires compaction
    void begin_push_mode(); // Switches to write/push mode

    // Commits write cache and primes read-ahead buffer from current head_ position
    void begin_pop_mode(); // Switches to read/pop mode

    // ------------------------------------------------------------------------
    // Core Queue APIs
    // ------------------------------------------------------------------------

    // Enqueues a packet of bytes (buffered in RAM cache until full or flushed)
    void push_back(std::vector<uint8_t> data); // Appends a byte packet to the FIFO

    // Dequeues the oldest unread packet from the head of the FIFO
    std::vector<uint8_t> Pop(); // Extracts next packet and advances persistent head_

    // Commits unwritten RAM cache to flash and persists the current head pointer
    void flush(); // Commits all dirty buffers and header state to disk

    // Checks whether the FIFO contains any unread packets
    bool empty(); // Returns true if queue has no pending items, false otherwise

private:
    std::string fileName_; // Logical relative filename of the FIFO file
    FileManager::SingleFile file_; // Level 2 persistent file handle managing low-level I/O

    std::vector<uint8_t> cache_; // In-memory buffer (write buffer in push, read-ahead in pop)
    size_t cachePos_ = 0; // Current read offset cursor within cache_ during pop mode
    uint32_t head_ = 0; // Absolute byte offset of the next unread packet in file

    bool pushMode_ = false; // Flag indicating if push mode is currently active
    bool popMode_ = false; // Flag indicating if pop mode is currently active

    // ------------------------------------------------------------------------
    // On-Disk Layout & Threshold Constants
    // ------------------------------------------------------------------------
    static constexpr uint32_t FIFO_MAGIC = 0x4F464946; // Magic bytes "FIFO" in ASCII little-endian
    static constexpr size_t HEADER_SIZE = 8; // 4 bytes Magic + 4 bytes head pointer = 8 bytes
    static constexpr size_t CACHE_CAPACITY = 1024; // Maximum in-memory RAM cache capacity (1 KB)
    static constexpr size_t GAP_THRESHOLD = 4096; // Shift only when dead space exceeds 4 KB
    static constexpr size_t MAX_FILE_THRESHOLD = 32768; // Shift if total file size exceeds 32 KB

    // ------------------------------------------------------------------------
    // Internal Helper Methods
    // ------------------------------------------------------------------------
    void initHeader(); // Formats new file or loads existing head_ on boot
    void writeHeadToDisk(bool forceFlush = false); // Saves head_ to offset 4 of the file
    void maybeCompact(); // Evaluates gap size and only shifts flash if thresholds are exceeded
    void compactDisk(); // Performs physical flash byte shift and truncation
    bool refillPopCache(); // Loads next chunk of data from disk starting at head_
}; // Closes FIFO class definition

} // namespace file // Closes namespace