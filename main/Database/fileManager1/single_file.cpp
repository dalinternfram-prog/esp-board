

#include "single_file.hpp" // Includes SingleFile class declaration
#include "esp_log.h"       // ESP-IDF logging library
#include <sys/unistd.h>    // POSIX ftruncate() and unlink()
#include <algorithm>       // std::min and std::max
#include <cerrno>          // System errno variable
#include <cstring>         // strerror()
#include <sys/stat.h>      // POSIX stat and fstat interfaces
#include <memory>          // std::unique_ptr for safe heap transfer buffers

namespace FileManager { // Opens FileManager namespace implementation

static const char* TAG = "SingleFile"; // Log filter tag used in ESP_LOG output

SingleFile::SingleFile(const std::string& filename) // Constructor binding single file
    : m_filename(filename), // Saves logical filename string
      m_fullPath(getFullPath(filename)) { // Resolves absolute VFS mount path using Level 1 helper
    openInternal(); // Automatically opens file so caller doesn't have to manage open states
} // Closes constructor definition

SingleFile::~SingleFile() { // Destructor
    if (m_file) { // Checks if file descriptor is open
        fflush(m_file); // Flushes any pending uncommitted data to flash
        fclose(m_file); // Releases the underlying system file descriptor
        m_file = nullptr; // Clears the pointer
    } // Closes open handle check
} // Closes destructor definition

SingleFile::SingleFile(SingleFile&& other) noexcept // Move constructor
    : m_file(other.m_file), // Takes ownership of other's file pointer
      m_filename(std::move(other.m_filename)), // Takes ownership of other's filename string
      m_fullPath(std::move(other.m_fullPath)) { // Takes ownership of other's full path string
    other.m_file = nullptr; // Nullifies other's pointer to prevent it from closing the file
} // Closes move constructor definition

SingleFile& SingleFile::operator=(SingleFile&& other) noexcept { // Move assignment operator
    if (this != &other) { // Protects against self-assignment
        if (m_file) { // Checks if this instance currently holds an open file
            fflush(m_file); // Flushes current file buffers
            fclose(m_file); // Closes current file descriptor
        } // Closes open check
        m_file = other.m_file; // Adopts new file descriptor
        m_filename = std::move(other.m_filename); // Adopts new filename
        m_fullPath = std::move(other.m_fullPath); // Adopts new full path
        other.m_file = nullptr; // Invalidates source object's pointer
    } // Closes self-assignment check
    return *this; // Returns reference to self
} // Closes move assignment definition

bool SingleFile::openInternal() { // Internal helper that transparently manages opening
    if (m_file) return true; // File is already open; reuse handle directly

    m_file = fopen(m_fullPath.c_str(), "r+b"); // Attempts to open existing file for read/update
    if (!m_file) { // If open fails, the file likely does not exist yet
        m_file = fopen(m_fullPath.c_str(), "w+b"); // Fall back to create and open for read/write
    } // Closes fallback check

    if (!m_file) { // Checks if both attempts failed
        ESP_LOGE(TAG, "Failed to open or create file '%s' (errno %d: %s)", m_fullPath.c_str(), errno, strerror(errno)); // Logs failure
        return false; // Returns failure
    } // Closes validation check

    return true; // Returns success
} // Closes openInternal definition

bool SingleFile::seek(long offset, int whence) { // Internal cursor positioning wrapper
    if (!openInternal()) return false; // Ensures file is open prior to seeking
    return (fseek(m_file, offset, whence) == 0); // Performs fseek and returns true on success
} // Closes seek definition

size_t SingleFile::fileSize() {
    if (!openInternal()) return 0;

    // 1. Flush any pending libc buffers so ftell reflects all written bytes
    fflush(m_file);

    // 2. Remember current stream cursor
    long currentPos = ftell(m_file);

    // 3. Seek to EOF to measure length
    if (fseek(m_file, 0, SEEK_END) != 0) {
        ESP_LOGE(TAG, "fseek(SEEK_END) failed on '%s' (errno %d: %s)", m_fullPath.c_str(), errno, strerror(errno));
        return 0;
    }

    long size = ftell(m_file);

    // 4. Restore original cursor position so we don't disrupt user's read/write offset
    if (currentPos >= 0) {
        fseek(m_file, currentPos, SEEK_SET);
    }

    return (size >= 0) ? static_cast<size_t>(size) : 0;
}
// Zero-copy internal write helper shared by both std::string and std::vector overloads
static bool writeRaw(FILE* file, const uint8_t* dataPtr, size_t totalBytes, long offset, SingleFile* self) {
    // Avoid double fflush: self->fileSize() already flushes libc buffers to flash
    long currentSize = static_cast<long>(self->fileSize()); // Queries true file size

    // Map negative offset to append directly at EOF
    if (offset < 0) {
        offset = currentSize; // Clamps to end of file
    } // Closes negative offset handling

    // Offset exceeds EOF: fill intermediate gap with zeros to avoid undefined regions
    if (offset > currentSize) {
        if (!self->seek(0, SEEK_END)) { // Positions cursor to current EOF
            ESP_LOGE(TAG, "Failed seeking to EOF for padding (errno %d: %s)", errno, strerror(errno)); // Logs seek failure
            return false; // Aborts on seek failure
        } // Closes seek EOF check

        long gap = offset - currentSize; // Computes number of zero bytes to pad
        constexpr size_t ZERO_BUF_SIZE = 256; // Stack-safe zero padding buffer
        const uint8_t zeroBuffer[ZERO_BUF_SIZE] = {0}; // Zero-initialized memory block

        while (gap > 0) { // Pads until target offset is reached
            size_t toWrite = std::min(static_cast<size_t>(gap), ZERO_BUF_SIZE); // Clamps to buffer size
            size_t written = fwrite(zeroBuffer, 1, toWrite, file); // Writes zeros to flash
            if (written != toWrite) { // Checks for short write error
                ESP_LOGE(TAG, "Failed to write zero padding: wrote %zu of %zu (errno %d: %s)", written, toWrite, errno, strerror(errno)); // Logs error
                return false; // Aborts on write failure
            } // Closes write validation
            gap -= toWrite; // Decrements remaining gap
        } // Closes gap padding loop
    } else {
        // Seek to target write offset only if we didn't just pad up to it
        if (!self->seek(offset, SEEK_SET)) { // Moves file pointer to target write location
            ESP_LOGE(TAG, "Failed seeking to offset %ld (errno %d: %s)", offset, errno, strerror(errno)); // Logs seek failure
            return false; // Aborts on seek failure
        } // Closes seek check
    } // Closes offset branch

    // Write actual data in 4 KB chunks matching LittleFS sector/erase boundaries
    if (totalBytes > 0 && dataPtr != nullptr) { // Verifies non-empty payload
        size_t bytesWrittenTotal = 0; // Running total of written bytes
        constexpr size_t CHUNK_SIZE = 4096; // 4 KB chunk size matching LittleFS block structure

        while (bytesWrittenTotal < totalBytes) { // Chunks data to minimize flash fragmentation
            size_t chunkBytes = std::min(CHUNK_SIZE, totalBytes - bytesWrittenTotal); // Clamps chunk
            size_t written = fwrite(dataPtr + bytesWrittenTotal, 1, chunkBytes, file); // Performs actual write

            if (written != chunkBytes) { // Checks if driver accepted all chunk bytes
                ESP_LOGE(TAG, "Short write: wrote %zu of %zu bytes (total written: %zu, errno %d: %s)",
                         written, chunkBytes, bytesWrittenTotal, errno, strerror(errno)); // Logs error
                return false; // Aborts write sequence
            } // Closes short write check
            bytesWrittenTotal += written; // Advances total written count
        } // Closes chunk loop
    } // Closes payload check

    return true; // Write completed successfully
} // Closes writeRaw helper

bool SingleFile::writeFile(const std::string& data, long offset) { // String overload with ZERO heap allocation
    if (!openInternal()) return false; // Ensures file handle is active
    return writeRaw(m_file, reinterpret_cast<const uint8_t*>(data.data()), data.size(), offset, this); // Zero-copy delegation
} // Closes string writeFile definition

bool SingleFile::writeFile(const std::vector<uint8_t>& data, long offset) { // Binary vector overload
    if (!openInternal()) return false; // Ensures file handle is active
    return writeRaw(m_file, data.data(), data.size(), offset, this); // Directly forwards binary pointer
} // Closes vector writeFile definition

std::vector<uint8_t> SingleFile::readFileBytes(long offset, long size) { // Core read implementation
    if (!openInternal()) return {}; // Auto-opens file if needed; returns empty on fail

    long totalLength = static_cast<long>(fileSize()); // Queries file length (automatically flushed)
    if (totalLength <= 0) { // Checks if file is empty
        return {}; // Early exit without error if file has no content
    } // Closes empty file check

    if (offset < 0 || offset >= totalLength) { // Validates offset bounds
        ESP_LOGW(TAG, "Read offset %ld out of bounds for '%s' (file size: %ld)", offset, m_fullPath.c_str(), totalLength); // Warns of out-of-bounds read
        return {}; // Returns empty vector on invalid bounds
    } // Closes bounds check

    size_t bytesToRead = 0; // Initializes read counter
    if (size < 0) { // If negative size, read everything up to EOF
        bytesToRead = static_cast<size_t>(totalLength - offset); // Calculates remaining bytes
    } else { // Explicit read size requested
        bytesToRead = std::min(static_cast<size_t>(size), static_cast<size_t>(totalLength - offset)); // Clamps to EOF
    } // Closes size calculation

    if (bytesToRead == 0) { // Avoids allocating empty vector or seeking
        return {}; // Early return for zero-byte read requests
    } // Closes zero check

    if (!seek(offset, SEEK_SET)) { // Positions cursor to starting offset
        ESP_LOGE(TAG, "Failed seeking to offset %ld in '%s' (errno %d: %s)", offset, m_fullPath.c_str(), errno, strerror(errno)); // Logs seek failure
        return {}; // Returns empty if seek failed
    } // Closes seek check

    std::vector<uint8_t> result(bytesToRead); // Allocates output container only when ready to read
    size_t readCount = fread(result.data(), 1, bytesToRead, m_file); // Reads bytes from flash

    if (readCount != bytesToRead) { // Detects short read / unexpected premature EOF
        ESP_LOGW(TAG, "Short read on '%s': expected %zu bytes, read %zu bytes (errno %d: %s)",
                 m_fullPath.c_str(), bytesToRead, readCount, errno, strerror(errno)); // Warns caller of truncated read
        result.resize(readCount); // Resizes buffer down to actual bytes read
    } // Closes short read check

    return result; // Returns read byte data
} // Closes readFileBytes definition


void SingleFile::shiftBytes(size_t shiftDistance, bool left, long long startOffset, long long endOffset) {
    if (shiftDistance == 0) return;

    long currentFileSize = static_cast<long>(fileSize());
    if (currentFileSize <= 0) return;

    long actualStart = (startOffset < 0) ? 0 : static_cast<long>(startOffset);
    long actualEnd = (endOffset < 0 || endOffset > currentFileSize) ? currentFileSize : static_cast<long>(endOffset);

    if (actualStart >= currentFileSize || actualStart >= actualEnd) return;

    if (left && static_cast<long>(shiftDistance) > actualStart) {
        ESP_LOGE(TAG, "Cannot shift left: underflow offset 0");
        return;
    }

  
    // 1. Close current file handle properly
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }

    // Use your class's path variable/getter here:
    // (Replace `m_filename` with whichever variable holds the path in SingleFile, e.g., m_filePath or getPath())
    std::string filePath = m_fullPath; 
    std::string tmpPath = filePath + ".tmp";
 
    FILE* src = fopen(filePath.c_str(), "rb");
    FILE* dst = fopen(tmpPath.c_str(), "wb");
    if (!src || !dst) {
                ESP_LOGE(TAG, "shiftBytes fopen failed! src: %p, dst: %p, path: '%s' (errno %d: %s)",
                 src, dst, filePath.c_str(), errno, strerror(errno));
        if (src) fclose(src);
        if (dst) fclose(dst);
        openInternal();
        return;
    }

    constexpr size_t BUF_SIZE = 4096; // 4 KB matches flash block size
    auto buf = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[BUF_SIZE]);
    if (!buf) {
        fclose(src);
        fclose(dst);
        openInternal();
        return;
    }

    auto copyRange = [&](long from, long to) {
        fseek(src, from, SEEK_SET);
        long remaining = to - from;
        while (remaining > 0) {
            size_t n = std::min<size_t>(remaining, BUF_SIZE);
            size_t r = fread(buf.get(), 1, n, src);
            if (r == 0) break;
            fwrite(buf.get(), 1, r, dst);
            remaining -= r;
        }
    };

    if (left) {
        // 1. Data before the destination: [0, actualStart - shiftDistance)
        if (actualStart > static_cast<long>(shiftDistance)) {
            copyRange(0, actualStart - shiftDistance);
        }
        // 2. The shifted payload: [actualStart, actualEnd) -> written sequentially!
        copyRange(actualStart, actualEnd);
        // 3. Any trailing data after actualEnd: [actualEnd, currentFileSize)
        if (actualEnd < currentFileSize) {
            copyRange(actualEnd, currentFileSize);
        }
    } else {
        // 1. Unchanged prefix: [0, actualStart)
        if (actualStart > 0) {
            copyRange(0, actualStart);
        }
        // 2. The insertion hole/gap: filled with zeros
        std::vector<uint8_t> zeros(std::min<size_t>(shiftDistance, BUF_SIZE), 0x00);
        size_t holeLeft = shiftDistance;
        while (holeLeft > 0) {
            size_t n = std::min(holeLeft, zeros.size());
            fwrite(zeros.data(), 1, n, dst);
            holeLeft -= n;
        }
        // 3. The shifted payload: [actualStart, actualEnd)
        copyRange(actualStart, actualEnd);
        // 4. Any trailing data after actualEnd: [actualEnd, currentFileSize)
        if (actualEnd < currentFileSize) {
            copyRange(actualEnd, currentFileSize);
        }
    }

    fflush(dst);
    fclose(src);
    fclose(dst);

    // Atomic replace on LittleFS
    unlink(filePath.c_str());
    rename(tmpPath.c_str(), filePath.c_str());

    // Reopen handle for caller
    openInternal();
}
bool SingleFile::truncate(size_t newSize) { // Truncates or extends file using LittleFS ftruncate
    if (!openInternal()) return false; // Auto-opens file if needed
    fflush(m_file); // Flushes stdio stream buffers before applying low-level truncate
    if (ftruncate(fileno(m_file), newSize) != 0) { // Invokes POSIX ftruncate via native file descriptor
        ESP_LOGE(TAG, "ftruncate to size %zu failed on '%s' (errno %d: %s)", newSize, m_fullPath.c_str(), errno, strerror(errno)); // Logs truncate failure
        return false; // Returns failure
    } // Closes ftruncate check
    return true; // Returns success
} // Closes truncate definition

bool SingleFile::deleteFile() { // Deletes this file
    if (m_file) { // If handle is active
        fclose(m_file); // Must close handle first before deleting file on filesystem
        m_file = nullptr; // Resets pointer to null
    } // Closes active handle check
    return FileManager::deleteFile(m_filename); // Invokes Level 1 unlink method
} // Closes deleteFile definition

void SingleFile::flush() { // Flushes pending unwritten data to flash
    if (m_file) { // Protects against null-pointer call which would flush all global streams
        fflush(m_file); // Flushes only this specific file stream
    } // Closes null check
} // Closes flush definition

} // namespace FileManager // Closes namespace