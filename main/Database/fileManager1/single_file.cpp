#include "single_file.hpp" // Includes SingleFile class declaration
#include "esp_log.h"       // ESP-IDF logging library
#include <sys/unistd.h>    // POSIX ftruncate() and unlink()
#include <algorithm>       // std::min and std::max
#include <cerrno>          // System errno variable

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
        ESP_LOGE(TAG, "Failed to open or create file '%s' (errno: %d)", m_fullPath.c_str(), errno); // Logs failure
        return false; // Returns failure
    } // Closes validation check

    return true; // Returns success
} // Closes openInternal definition

bool SingleFile::seek(long offset, int whence) { // Internal cursor positioning wrapper
    if (!openInternal()) return false; // Ensures file is open prior to seeking
    return (fseek(m_file, offset, whence) == 0); // Performs fseek and returns true on success
} // Closes seek definition

size_t SingleFile::fileSize() { // Computes current size of this file
    if (!openInternal()) return 0; // Return zero if file cannot be opened
    long cur = ftell(m_file); // Saves current read/write cursor position
    fseek(m_file, 0, SEEK_END); // Seeks to the end of the file
    long end = ftell(m_file); // Records offset at the file end
    fseek(m_file, cur, SEEK_SET); // Restores cursor to original position
    return (end < 0) ? 0 : static_cast<size_t>(end); // Returns non-negative byte count
} // Closes fileSize definition

bool SingleFile::writeFile(const std::string& data, long offset) { // String overload
    return writeFile(std::vector<uint8_t>(data.begin(), data.end()), offset); // Forwards to binary write method
} // Closes string writeFile definition

bool SingleFile::writeFile(const std::vector<uint8_t>& data, long offset) { // Binary write implementation
    if (!openInternal()) return false; // Auto-opens file if needed

    long currentSize = static_cast<long>(fileSize()); // Queries current total file length

    if (offset < 0) { // Checks if append mode was requested
        offset = currentSize; // Maps negative offset to current end-of-file
    } // Closes append check

    if (offset > currentSize) { // Offset exceeds EOF: fill intermediate gap with zeros
        seek(0, SEEK_END); // Moves cursor to current end of data
        long gap = offset - currentSize; // Calculates number of zero bytes needed
        constexpr size_t ZERO_BUF_SIZE = 256; // Buffer size for zero-chunk writes
        const uint8_t zeroBuffer[ZERO_BUF_SIZE] = {0}; // Static zero array

        while (gap > 0) { // Loops until entire gap is filled
            size_t toWrite = std::min(static_cast<size_t>(gap), ZERO_BUF_SIZE); // Clamps to buffer size
            size_t written = fwrite(zeroBuffer, 1, toWrite, m_file); // Writes zeroes to file
            if (written != toWrite) { // Checks if zero write encountered an error
                ESP_LOGE(TAG, "Failed to write zero padding"); // Logs error message
                return false; // Returns failure
            } // Closes write error check
            gap -= toWrite; // Deducts written bytes from remaining gap
        } // Closes gap filling loop
    } // Closes gap check

    if (!seek(offset, SEEK_SET)) { // Moves cursor to target write offset
        ESP_LOGE(TAG, "Failed seeking to offset %ld", offset); // Logs seek error
        return false; // Returns failure
    } // Closes seek check

    if (!data.empty()) { // Only write if input buffer contains bytes
        size_t written = fwrite(data.data(), 1, data.size(), m_file); // Writes actual data bytes to flash
        if (written != data.size()) { // Verifies all bytes were committed
            ESP_LOGE(TAG, "Short write: wrote %u of %u bytes", (unsigned)written, (unsigned)data.size()); // Logs short write
            return false; // Returns failure
        } // Closes short write check
    } // Closes empty check

    return true; // Successfully wrote data
} // Closes binary writeFile definition

std::vector<uint8_t> SingleFile::readFileBytes(long offset, long size) { // Core read implementation
    if (!openInternal()) return {}; // Auto-opens file if needed; returns empty on fail

    long totalLength = static_cast<long>(fileSize()); // Queries file length
    if (totalLength <= 0 || offset < 0 || offset >= totalLength) { // Validates offset bounds
        return {}; // Returns empty vector if file is empty or offset out of bounds
    } // Closes bounds check

    size_t bytesToRead = 0; // Initializes read counter
    if (size < 0) { // If negative size, read everything up to EOF
        bytesToRead = static_cast<size_t>(totalLength - offset); // Calculates remaining bytes
    } else { // Explicit read size requested
        bytesToRead = std::min(static_cast<size_t>(size), static_cast<size_t>(totalLength - offset)); // Clamps to EOF
    } // Closes size calculation

    std::vector<uint8_t> result(bytesToRead); // Allocates buffer for requested byte chunk

    if (bytesToRead > 0) { // Checks if there are bytes to read
        if (!seek(offset, SEEK_SET)) { // Positions cursor to starting offset
            return {}; // Returns empty if seek failed
        } // Closes seek check
        size_t readCount = fread(result.data(), 1, bytesToRead, m_file); // Reads bytes from flash
        if (readCount != bytesToRead) { // Checks if read returned fewer bytes than expected
            result.resize(readCount); // Resizes buffer down to actual bytes read
        } // Closes resize check
    } // Closes read execution block

    return result; // Returns read byte data
} // Closes readFileBytes definition

void SingleFile::shiftBytes(size_t shiftDistance, bool left, long long startOffset, long long endOffset) { // Shift implementation
    if (!openInternal() || shiftDistance == 0) return; // Fail fast if file cannot be opened or distance is 0

    long currentFileSize = static_cast<long>(fileSize()); // Queries current file size
    if (currentFileSize <= 0) return; // Cannot shift an empty file

    long actualStart = (startOffset < 0) ? 0 : static_cast<long>(startOffset); // Default startOffset < 0 to 0
    long actualEnd = (endOffset < 0 || endOffset > currentFileSize) ? currentFileSize : static_cast<long>(endOffset); // Default end to EOF

    if (actualStart >= currentFileSize || actualStart >= actualEnd) return; // Validates range bounds

    constexpr size_t CHUNK_SIZE = 512; // Buffer chunk size conservative for embedded RAM
    uint8_t buffer[CHUNK_SIZE]; // Temporary byte transfer buffer

    if (left) { // Left shift (compaction forward)
        long currentSrc = actualStart; // Source cursor starts at beginning of target region
        long currentDst = std::max(0L, actualStart - static_cast<long>(shiftDistance)); // Destination shifted left

        while (currentSrc < actualEnd) { // Iterates forward through region to prevent overwriting unread data
            size_t bytesToRead = std::min(CHUNK_SIZE, static_cast<size_t>(actualEnd - currentSrc)); // Clamps chunk
            seek(currentSrc, SEEK_SET); // Seeks to source position
            size_t readBytes = fread(buffer, 1, bytesToRead, m_file); // Reads source chunk
            if (readBytes == 0) break; // Reached unexpected EOF; break

            seek(currentDst, SEEK_SET); // Seeks to destination position
            if (fwrite(buffer, 1, readBytes, m_file) != readBytes) { // Writes chunk into new location
                ESP_LOGE(TAG, "Left shift write failed"); // Logs write failure
                break; // Abort on error
            } // Closes write error check

            currentSrc += readBytes; // Advances source offset
            currentDst += readBytes; // Advances destination offset
        } // Closes left shift loop
    } else { // Right shift (expansion backward)
        long currentSrc = actualEnd; // Source cursor starts at end of target region

        while (currentSrc > actualStart) { // Iterates backward through region to prevent overwriting unread data
            size_t bytesToRead = std::min(CHUNK_SIZE, static_cast<size_t>(currentSrc - actualStart)); // Clamps chunk
            currentSrc -= bytesToRead; // Decrements source offset to chunk start

            seek(currentSrc, SEEK_SET); // Seeks to chunk start
            size_t readBytes = fread(buffer, 1, bytesToRead, m_file); // Reads source chunk
            if (readBytes == 0) break; // Reached unexpected EOF; break

            long currentDst = currentSrc + static_cast<long>(shiftDistance); // Calculates right-shifted destination
            seek(currentDst, SEEK_SET); // Seeks to shifted destination
            if (fwrite(buffer, 1, readBytes, m_file) != readBytes) { // Writes chunk into new location
                ESP_LOGE(TAG, "Right shift write failed"); // Logs write failure
                break; // Abort on error
            } // Closes write error check
        } // Closes right shift loop
    } // Closes left/right branch

    fflush(m_file); // Flushes all shifted blocks to physical flash
} // Closes shiftBytes definition

bool SingleFile::truncate(size_t newSize) { // Truncates or extends file
    if (!openInternal()) return false; // Auto-opens file if needed
    fflush(m_file); // Flushes stdio stream buffers before applying low-level truncate
    return (ftruncate(fileno(m_file), newSize) == 0); // Invokes POSIX ftruncate via native file descriptor
} // Closes truncate definition

bool SingleFile::deleteFile() { // Deletes this file
    if (m_file) { // If handle is active
        fclose(m_file); // Must close handle first before deleting file on SPIFFS
        m_file = nullptr; // Resets pointer to null
    } // Closes active handle check
    return FileManager::deleteFile(m_filename); // Invokes Level 1 unlink method
} // Closes deleteFile definition
void SingleFile::flush(){
 fflush(m_file); 
}
} // namespace FileManager // Closes namespace