#pragma once
#include "esp_spiffs.h"
#include <string>
#include "esp_err.h"
#include <vector>
class FileManager {
public:
    FileManager();
    ~FileManager();

    esp_err_t init(const std::string& base_path = "/spiffs", const std::string& partition_label = "my_files");

    esp_err_t deinit();

/**
     * @brief Writes a string to a file starting at a specified byte offset.
     * 
     * @param filename Path or name of the file relative to the storage mount point.
     * @param data     The string buffer to write.
     * @param offset   Byte offset to begin writing at:
     *                 - `>= 0`: Seeks to the specific offset. If the offset exceeds the current 
     *                   file size, the gap between the original end-of-file and the offset is 
     *                   automatically padded with zeros (`\0`).
     *                 - `< 0`: Operates in append mode (writes directly at the current end-of-file).
     * @return true on success, false if opening, seeking, padding, or writing failed.
     */
    bool writeFile(const std::string& filename, const std::string& data, long offset = 0);

    /**
     * @brief Overload wrapper to write binary byte data (std::vector<uint8_t>) to a file.
     * 
     * @param filename Path or name of the file relative to the storage mount point.
     * @param data     Raw binary byte buffer to write.
     * @param offset   Byte offset to begin writing at (same behavior as the string overload:
     *                 >= 0 for explicit offset with zero-padding, < 0 for append mode).
     * @return true on success, false on failure.
     */
    bool writeFile(const std::string& filename, const std::vector<uint8_t>& data, long offset = 0);

    /**
     * @brief Reads a specific chunk of binary data from a file.
     * 
     * @param filename Path or name of the file relative to the storage mount point.
     * @param offset   The starting byte offset from the beginning of the file (must be >= 0).
     * @param size     The number of bytes to read:
     *                 - `>= 0`: Reads up to `size` bytes (automatically clamped if it exceeds EOF).
     *                 - `< 0`: Reads all remaining bytes from the `offset` until the end of the file.
     * @return std::vector<uint8_t> Containing the read bytes, or an empty vector if the file 
     *         cannot be opened, the offset is out of bounds, or an error occurs.
     */
    std::vector<uint8_t> readFileBytes(const std::string& filename, long offset, long size = -1);

    /**
     * @brief Shifts a block of bytes left or right within a file in-place (no temporary files).
     * 
     * @param filename       Path or name of the file relative to the storage mount point.
     * @param shiftDistance  The number of bytes to shift the target region.
     * @param left           `true` to shift bytes left (forward compaction), 
     *                       `false` to shift bytes right (backward expansion).
     * @param startOffset    Start of the region to shift. Pass a negative value (`-1`) to default to 0.
     * @param endOffset      End of the region to shift. Pass a negative value (`-1`) to default to EOF.
     */
    void shiftBytes(const std::string& filename, size_t shiftDistance, bool left, long long startOffset = -1, long long endOffset = -1);

bool truncate(const std::string& path, size_t newSize);

    bool deleteFile(const std::string& filename);
    bool exists(const std::string& filename);
    bool getStorageStats(size_t* total_bytes, size_t* free_bytes); 
private:
    std::string m_base_path;
    std::string m_partition_label;
    bool m_is_mounted;
    std::string getFullPath(const std::string& filename) const;
};