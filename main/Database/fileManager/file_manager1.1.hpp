// #pragma once // Prevent this header from being included more than once.
// #include <cstddef> // Declare size_t for byte counts and storage statistics.
// #include <cstdint> // Declare uint8_t for binary data.
// #include <mutex> // Provide a mutex shared by all FileManager operations.
// #include <string> // Declare std::string for paths and text data.
// #include <vector> // Declare std::vector for binary data buffers.
// #include "esp_err.h" // Declare esp_err_t and ESP-IDF error codes.

// namespace FileManager { // Keep storage functions in a dedicated namespace.

//     // Mount LittleFS at base_path using the partition named partition_label.
//     esp_err_t init(const std::string& base_path = "/littlefs", const std::string& partition_label = "my_files"); // Keep the old configurable initialization interface.

//     // Unmount LittleFS; callers must stop all users of this module before deinitializing.
//     esp_err_t deinit(); // Return the ESP-IDF unmount status.

//     /**
//      * @brief Write text bytes at an explicit offset or append when offset is negative.
//      * @param filename File name relative to the mount point, or a path beginning with '/'.
//      * @param data Text bytes to write; embedded NUL bytes are also written because size() is used.
//      * @param offset Non-negative offsets overwrite from that position and zero-pad gaps; negative offsets append.
//      * @return true if the complete write and close succeeded; otherwise false.
//      */
//     bool writeFile(const std::string& filename, const std::string& data, long offset = 0); // Preserve the existing text overload.

//     /**
//      * @brief Write binary bytes at an explicit offset or append when offset is negative.
//      * @param filename File name relative to the mount point, or a path beginning with '/'.
//      * @param data Binary bytes to write without converting them to text.
//      * @param offset Non-negative offsets overwrite from that position and zero-pad gaps; negative offsets append.
//      * @return true if the complete write and close succeeded; otherwise false.
//      */
//     bool writeFile(const std::string& filename, const std::vector<uint8_t>& data, long offset = 0); // Preserve the existing binary overload.

//     /**
//      * @brief Read a byte range from a file.
//      * @param filename File name relative to the mount point, or a path beginning with '/'.
//      * @param offset Non-negative starting byte offset.
//      * @param size Number of bytes to read; a negative value means read to EOF.
//      * @return The bytes read; an empty vector also represents open/seek/read failure or an empty range.
//      */
//     std::vector<uint8_t> readFileBytes(const std::string& filename, long offset, long size = -1); // Preserve the existing read interface.

//     /**
//      * @brief Shift a byte range left or right in-place using bounded temporary buffers.
//      * @param filename File name relative to the mount point, or a path beginning with '/'.
//      * @param shiftDistance Number of bytes to shift.
//      * @param left True shifts toward lower offsets; false shifts toward higher offsets.
//      * @param startOffset Start of the region; a negative value means zero.
//      * @param endOffset Exclusive end of the region; a negative value means the current EOF.
//      * @note This multi-write operation is not atomic if power fails midway through the shift.
//      */
//     void shiftBytes(const std::string& filename, size_t shiftDistance, bool left, long long startOffset = -1, long long endOffset = -1); // Preserve the existing shift interface.

//     // Resize a file to newSize bytes; extending a file typically reads as zero-filled bytes.
//     bool truncate(const std::string& path, size_t newSize); // Preserve the existing truncate interface.

//     // Delete one file; returns false if the operation fails.
//     bool deleteFile(const std::string& filename); // Preserve the existing delete interface.

//     // Return true when stat() can find the requested file.
//     bool exists(const std::string& filename); // Preserve the existing existence-check interface.

//     // Return total and free bytes for the mounted LittleFS partition.
//     bool getStorageStats(size_t* total_bytes, size_t* free_bytes); // Preserve the existing storage-statistics interface.

// } // namespace FileManager // End the FileManager namespace.