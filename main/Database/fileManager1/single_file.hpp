#pragma once // Ensures this header file is included only once during compilation

#include "file_manager.hpp" // Imports Level 1 filesystem base utilities (getFullPath, deleteFile)
#include <string>           // Provides std::string for data and path representation
#include <vector>           // Provides std::vector for raw binary byte buffers
#include <cstdint>          // Provides standard integer types such as uint8_t
#include <cstdio>           // Provides standard C file I/O types (FILE*)
#include <cstddef>          // Provides standard size types such as size_t

namespace FileManager { // Encloses the class within the FileManager namespace

class SingleFile { // Dedicated object encapsulating persistent state and I/O for a single file
public:
    // Binds this instance to a specific file and automatically opens/prepares it for I/O
    explicit SingleFile(const std::string& filename); // Caller supplies the filename only once here

    // Destructor that automatically flushes unwritten data and closes the open descriptor (RAII)
    ~SingleFile(); // Ensures the file handle is safely closed when the object goes out of scope

    // Disable copy semantics to prevent two instances from holding and closing the same FILE* handle
    SingleFile(const SingleFile&) = delete; // Prevents copying of the file instance
    SingleFile& operator=(const SingleFile&) = delete; // Prevents copy assignment

    // Enable move semantics to allow transferring file handle ownership between functions/scopes
    SingleFile(SingleFile&& other) noexcept; // Move constructor transfers pointer and path
    SingleFile& operator=(SingleFile&& other) noexcept; // Move assignment operator transfers ownership

    // ------------------------------------------------------------------------
    // Core APIs
    // ------------------------------------------------------------------------

    // Writes string data starting at offset (>= 0 for seek with zero-padding, < 0 for append)
    bool writeFile(const std::string& data, long offset = 0); // Writes string buffer to file

    // Writes binary byte buffer starting at offset (>= 0 for seek with zero-padding, < 0 for append)
    bool writeFile(const std::vector<uint8_t>& data, long offset = 0); // Writes byte array to file

    // Reads binary bytes starting from offset (size < 0 reads until end-of-file)
    std::vector<uint8_t> readFileBytes(long offset = 0, long size = -1); // Reads chunk from file

    // Shifts a region of bytes left (compaction) or right (expansion) in-place without temp files
    void shiftBytes(size_t shiftDistance, bool left, long long startOffset = -1, long long endOffset = -1); // Moves byte ranges

    // Truncates or extends the file to the specified byte size
    bool truncate(size_t newSize); // Adjusts physical file size

    // Closes the internal handle and deletes the file from storage
    bool deleteFile(); // Deletes this file from flash storage

    // Computes and returns the current total byte size of the file
    size_t fileSize(); // Returns file size in bytes
    void flush();
private:
    FILE* m_file = nullptr; // Persistent standard C file stream handle
    std::string m_filename; // Relative filename bound to this object
    std::string m_fullPath; // Resolved full VFS path (e.g., "/spiffs/my_file.bin")

    bool openInternal(); // Internal helper to open or create file if not currently open
    bool seek(long offset, int whence = SEEK_SET); // Internal helper to position file cursor
}; // Closes SingleFile class declaration

} // namespace FileManager // Closes namespace