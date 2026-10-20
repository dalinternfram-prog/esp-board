// #include "file_manager.hpp"
// #include "esp_log.h"
// #include "esp_spiffs.h"
// #include <sys/unistd.h>
// #include <sys/stat.h>

// static const char* TAG = "FileManager";

// FileManager::FileManager() : m_base_path("/spiffs"), m_partition_label("my_files"), m_is_mounted(false) {}

// FileManager::~FileManager() {
//     if (m_is_mounted) {
//         deinit();
//     }
// }

// esp_err_t FileManager::init(const std::string& base_path, const std::string& partition_label) {
//     m_base_path = base_path;
//     m_partition_label = partition_label;

//     ESP_LOGI(TAG, "Initializing SPIFFS storage partition [%s] at [%s]", m_partition_label.c_str(), m_base_path.c_str());

//     esp_vfs_spiffs_conf_t conf = {
//         .base_path = m_base_path.c_str(),
//         .partition_label = m_partition_label.c_str(),
//         .max_files = 5,
//         .format_if_mount_failed = true // Automatically formats partition if first boot or corrupted
//     };

//     esp_err_t ret = esp_vfs_spiffs_register(&conf);

//     if (ret != ESP_OK) {
//         if (ret == ESP_FAIL) {
//             ESP_LOGE(TAG, "Failed to mount or format filesystem");
//         } else if (ret == ESP_ERR_NOT_FOUND) {
//             ESP_LOGE(TAG, "Failed to find SPIFFS partition '%s'. Check your partitions.csv!", m_partition_label.c_str());
//         } else {
//             ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
//         }
//         return ret;
//     }

//     m_is_mounted = true;
//     size_t total = 0, free = 0;
//     if (getStorageStats(&total, &free)) {
//         ESP_LOGI(TAG, "SPIFFS Mounted Successfully. Total: %u bytes, Free: %u bytes", total, free);
//     }

//     return ESP_OK;
// }

// esp_err_t FileManager::deinit() {
//     if (!m_is_mounted) return ESP_OK;

//     esp_err_t ret = esp_vfs_spiffs_unregister(m_partition_label.c_str());
//     if (ret == ESP_OK) {
//         m_is_mounted = false;
//         ESP_LOGI(TAG, "SPIFFS unmounted successfully");
//     }
//     return ret;
// }

// std::string FileManager::getFullPath(const std::string& filename) const {
//     if (filename.empty()) return m_base_path;
//     if (filename[0] == '/') {
//         return m_base_path + filename;
//     }
//     return m_base_path + "/" + filename;
// }

// bool FileManager::writeFile(const std::string& filename, const std::string& data) {
//     std::string path = getFullPath(filename);
//     FILE* f = fopen(path.c_str(), "w");
//     if (f == NULL) {
//         ESP_LOGE(TAG, "Failed to open file for writing: %s", path.c_str());
//         return false;
//     }
    
//     size_t written = fwrite(data.c_str(), 1, data.size(), f);
//     fclose(f);

//     if (written != data.size()) {
//         ESP_LOGE(TAG, "Short write: wrote %d of %d bytes", (int)written, (int)data.size());
//         return false;
//     }
//     return true;
// }

// bool FileManager::appendFile(const std::string& filename, const std::string& data) {
//     std::string path = getFullPath(filename);
//     FILE* f = fopen(path.c_str(), "a");
//     if (f == NULL) {
//         ESP_LOGE(TAG, "Failed to open file for appending: %s", path.c_str());
//         return false;
//     }
    
//     fwrite(data.c_str(), 1, data.size(), f);
//     fclose(f);
//     return true;
// }

// std::string FileManager::readFile(const std::string& filename) {
//     std::string path = getFullPath(filename);
//     FILE* f = fopen(path.c_str(), "r");
//     if (f == NULL) {
//         ESP_LOGW(TAG, "Failed to open file for reading: %s", path.c_str());
//         return "";
//     }

//     fseek(f, 0, SEEK_END);
//     long length = ftell(f);
//     fseek(f, 0, SEEK_SET);

//     std::string content;
//     if (length > 0) {
//         content.resize(length);
//         fread(&content[0], 1, length, f);
//     }
//     fclose(f);
//     return content;
// }

// bool FileManager::deleteFile(const std::string& filename) {
//     std::string path = getFullPath(filename);
//     if (unlink(path.c_str()) != 0) {
//         ESP_LOGE(TAG, "Failed to delete file: %s", path.c_str());
//         return false;
//     }
//     return true;
// }

// bool FileManager::exists(const std::string& filename) {
//     std::string path = getFullPath(filename);
//     struct stat st;
//     return stat(path.c_str(), &st) == 0;
// }

// bool FileManager::getStorageStats(size_t* total_bytes, size_t* free_bytes) {
//     if (!m_is_mounted) return false;
//     size_t total = 0, used = 0;
//     if (esp_spiffs_info(m_partition_label.c_str(), &total, &used) == ESP_OK) {
//         *total_bytes = total;
//         *free_bytes = (total > used) ? (total - used) : 0;
//         return true;
//     }
//     return false;
// }



#include "file_manager.hpp"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/unistd.h>
#include <sys/stat.h>

// Tag string used to filter and identify log output originating from this module
static const char* TAG = "FileManager";

// Constructor: Initializes members with default mount path, partition label, and unmounted status
FileManager::FileManager() : m_base_path("/spiffs"), m_partition_label("my_files"), m_is_mounted(false) {}

// Destructor: Automatically unmounts SPIFFS during cleanup if it is currently mounted
FileManager::~FileManager() {
    if (m_is_mounted) {
        deinit(); // Ensure VFS resources and SPIFFS handle are properly released
    }
}

// Mounts the SPIFFS filesystem and registers it with ESP-IDF's Virtual Filesystem (VFS)
esp_err_t FileManager::init(const std::string& base_path, const std::string& partition_label) {
    m_base_path = base_path;             // Save custom base path (e.g., "/spiffs")
    m_partition_label = partition_label; // Save CSV partition label name

    ESP_LOGI(TAG, "Initializing SPIFFS storage partition [%s] at [%s]", m_partition_label.c_str(), m_base_path.c_str());

    // Configure the SPIFFS driver parameters
    esp_vfs_spiffs_conf_t conf = {
        .base_path = m_base_path.c_str(),
        .partition_label = m_partition_label.c_str(),
        .max_files = 5,                         // Maximum concurrent open files allowed
        .format_if_mount_failed = true          // Auto-formats flash partition on corrupt or unformatted state
    };

    // Attempt to register and mount SPIFFS into the Virtual Filesystem layer
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition '%s'. Check your partitions.csv!", m_partition_label.c_str());
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret; // Return error code immediately on failure
    }

    m_is_mounted = true; // Mark filesystem state as mounted
    size_t total = 0, free = 0;
    
    // Log available and total memory stats upon successful mount
    if (getStorageStats(&total, &free)) {
        ESP_LOGI(TAG, "SPIFFS Mounted Successfully. Total: %u bytes, Free: %u bytes", total, free);
    }

    return ESP_OK; // Initialization complete
}

// Safely unregisters the SPIFFS filesystem from the ESP VFS
esp_err_t FileManager::deinit() {
    if (!m_is_mounted) return ESP_OK; // No-op if filesystem isn't currently mounted

    esp_err_t ret = esp_vfs_spiffs_unregister(m_partition_label.c_str());
    if (ret == ESP_OK) {
        m_is_mounted = false; // Reset mount state flag
        ESP_LOGI(TAG, "SPIFFS unmounted successfully");
    }
    return ret;
}

// Constructs a complete VFS path by prepending the base path to relative input names
std::string FileManager::getFullPath(const std::string& filename) const {
    if (filename.empty()) return m_base_path; // Return base path if input filename is empty
    if (filename[0] == '/') {
        return m_base_path + filename;        // Concatenate directly if filename includes leading slash
    }
    return m_base_path + "/" + filename;      // Inject missing path separator between base and filename
}

// Writes string data to a file (overwrites existing contents)
bool FileManager::writeFile(const std::string& filename, const std::string& data) {
    std::string path = getFullPath(filename); // Resolve target absolute path
    FILE* f = fopen(path.c_str(), "w");        // Open file in write-truncate mode ("w")
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path.c_str());
        return false;
    }
    
    // Write full string buffer in a single chunk
    size_t written = fwrite(data.c_str(), 1, data.size(), f);
    fclose(f); // Always close the file stream after writing

    // Validate if the number of written bytes matches requested buffer length
    if (written != data.size()) {
        ESP_LOGE(TAG, "Short write: wrote %d of %d bytes", (int)written, (int)data.size());
        return false;
    }
    return true;
}

// Appends string data to the end of a file (creates file if non-existent)
bool FileManager::appendFile(const std::string& filename, const std::string& data) {
    std::string path = getFullPath(filename); // Resolve absolute filesystem path
    FILE* f = fopen(path.c_str(), "a");        // Open file in append-only mode ("a")
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending: %s", path.c_str());
        return false;
    }
    
    // Write chunk to end of file (Note: standard practice checks return byte count here as well)
    fwrite(data.c_str(), 1, data.size(), f);
    fclose(f); // Flush buffer and release file handle
    return true;
}
bool FileManager::writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::string s(data.begin(), data.end());
    return writeFile(filename, s);
}

bool FileManager::appendFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::string s(data.begin(), data.end());
    return appendFile(filename, s);
}

// Reads and returns the entire contents of a specified file into a std::string
std::string FileManager::readFile(const std::string& filename) {
    std::string path = getFullPath(filename); // Convert to complete mount path
    FILE* f = fopen(path.c_str(), "r");        // Open file stream for reading ("r")
    if (f == NULL) {
        ESP_LOGW(TAG, "Failed to open file for reading: %s", path.c_str());
        return ""; // Return empty string if target file cannot be opened
    }

    // Seek to end to derive total byte length
    fseek(f, 0, SEEK_END);
    long length = ftell(f); // Get current position offset (file size)
    fseek(f, 0, SEEK_SET); // Reset file pointer back to starting position

    std::string content;
    if (length > 0) {
        content.resize(length);              // Allocate exact memory in string buffer
        fread(&content[0], 1, length, f);    // Read entire content directly into allocated buffer memory
    }
    fclose(f); // Free file descriptor resource
    return content;
}
// Reads and returns the entire binary contents of a specified file into a std::vector<uint8_t>
std::vector<uint8_t> FileManager::readFileBytes(const std::string& filename) {
    std::string path = getFullPath(filename); // Convert to complete mount path
    FILE* f = fopen(path.c_str(), "rb");       // Open file stream for binary reading ("rb")
    if (f == NULL) {
        ESP_LOGW(TAG, "Failed to open file for reading: %s", path.c_str());
        return {}; // Return empty vector if target file cannot be opened
    }

    // Seek to end to derive total byte length
    fseek(f, 0, SEEK_END);
    long length = ftell(f); // Get current position offset (file size)
    fseek(f, 0, SEEK_SET); // Reset file pointer back to starting position

    std::vector<uint8_t> content;
    if (length > 0) {
        content.resize(length);               // Allocate exact memory in vector buffer
        fread(content.data(), 1, length, f);  // Read entire content directly into vector buffer memory
    }
    fclose(f); // Free file descriptor resource
    return content;
}

std::vector<uint8_t> FileManager::readFileBytes(const std::string& filename, size_t offset, size_t size) {
    std::string path = getFullPath(filename); // Convert to complete mount path
    FILE* f = fopen(path.c_str(), "rb");        // Open file stream for binary reading ("rb")
    if (f == NULL) {
        ESP_LOGW(TAG, "Failed to open file for reading: %s", path.c_str());
        return {}; // Return empty vector if target file cannot be opened
    }

    // Check total file size to make sure we don't read past the end
    fseek(f, 0, SEEK_END);
    long totalLength = ftell(f);
    if (totalLength < 0 || static_cast<size_t>(offset) >= static_cast<size_t>(totalLength)) {
        fclose(f);
        return {}; // Offset is out of bounds
    }

    // Adjust size if requested size exceeds available bytes from offset
    size_t bytesToRead = size;
    if (offset + bytesToRead > static_cast<size_t>(totalLength)) {
        bytesToRead = totalLength - offset;
    }

    std::vector<uint8_t> content;
    if (bytesToRead > 0) {
        // Seek directly to the target offset from the beginning of the file
        if (fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
            fclose(f);
            return {}; // Seek failed
        }

        content.resize(bytesToRead);              // Allocate memory ONLY for the requested chunk
        fread(content.data(), 1, bytesToRead, f); // Read only those specific bytes into memory
    }

    fclose(f); // Free file descriptor resource
    return content;
}





// Method to shift bytes within a file by a given shiftDistance (left or right) in constant memory chunks
void FileManager::shiftBytes(const std::string& filename, size_t shiftDistance, bool left) {
    if (shiftDistance == 0) return;

    std::string fullPath = getFullPath(filename);
    std::string tempPath = fullPath + ".tmp";
    const size_t CHUNK_SIZE = 2048; // Fixed 2KB chunk size to protect ESP32 RAM from OOM crashes
    size_t currentReadOffset = left ? shiftDistance : 0;
    bool firstChunk = true;

    // 1. Stream valid data into a temporary file with the applied shift
    while (true) {
        std::vector<uint8_t> chunk = readFileBytes(filename, currentReadOffset, CHUNK_SIZE);
        if (chunk.empty()) {
            break; // Reached the end of the file stream
        }

        if (firstChunk) {
            writeFile(filename + ".tmp", chunk); // Initialize temp file with first chunk
            firstChunk = false;
        } else {
            appendFile(filename + ".tmp", chunk); // Append subsequent chunks sequentially
        }

        currentReadOffset += chunk.size();
        if (chunk.size() < CHUNK_SIZE) {
            break; // Last partial chunk read
        }
    }

    // 2. Stream the compacted temporary file back to the original file
    currentReadOffset = 0;
    firstChunk = true;
    while (true) {
        std::vector<uint8_t> cleanChunk = readFileBytes(filename + ".tmp", currentReadOffset, CHUNK_SIZE);
        if (cleanChunk.empty()) {
            break;
        }

        if (firstChunk) {
            writeFile(filename, cleanChunk);
            firstChunk = false;
        } else {
            appendFile(filename, cleanChunk);
        }

        currentReadOffset += cleanChunk.size();
        if (cleanChunk.size() < CHUNK_SIZE) {
            break;
        }
    }

    // 3. Clean up the temporary file by clearing it out
    writeFile(filename + ".tmp", std::vector<uint8_t>());
}
 

// Method to shift bytes within a file by a given shiftDistance (left or right)
// within an optional range [startOffset, endOffset]. Pass negative values (-1) to use defaults.
void FileManager::shiftBytes(const std::string& filename, size_t shiftDistance, bool left, long long startOffset, long long endOffset) {
    if (shiftDistance == 0) return;

    std::string fullPath = getFullPath(filename);
    std::string tempPath = fullPath + ".tmp";
    const size_t CHUNK_SIZE = 2048; // Fixed 2KB chunk size to protect ESP32 RAM from OOM crashes

    size_t fileSize = getFileSize(filename);

    // Apply rules: if offset < 0, fallback to defaults (0 for start, fileSize for end)
    size_t actualStart = (startOffset < 0) ? 0 : static_cast<size_t>(startOffset);
    size_t actualEnd = (endOffset < 0 || static_cast<size_t>(endOffset) > fileSize) ? fileSize : static_cast<size_t>(endOffset);

    if (actualStart >= fileSize || actualStart >= actualEnd) return;

    // 1. Stream data into a temporary file:
    // - Prefix (0 to actualStart) is copied as-is
    size_t currentReadOffset = 0;
    while (currentReadOffset < actualStart) {
        size_t bytesToRead = std::min(CHUNK_SIZE, actualStart - currentReadOffset);
        std::vector<uint8_t> chunk = readFileBytes(filename, currentReadOffset, bytesToRead);
        if (chunk.empty()) break;

        if (currentReadOffset == 0) {
            writeFile(tempPath, chunk);
        } else {
            appendFile(tempPath, chunk);
        }
        currentReadOffset += chunk.size();
    }

    // - Target region [actualStart, actualEnd] with shift applied
    size_t targetStart = left ? (actualStart + shiftDistance) : actualStart;
    if (targetStart > actualEnd) targetStart = actualEnd;

    currentReadOffset = targetStart;
    bool firstTargetChunk = (actualStart == 0 && currentReadOffset == 0);

    while (currentReadOffset < actualEnd) {
        size_t bytesToRead = std::min(CHUNK_SIZE, actualEnd - currentReadOffset);
        std::vector<uint8_t> chunk = readFileBytes(filename, currentReadOffset, bytesToRead);
        if (chunk.empty()) break;

        if (firstTargetChunk && actualStart == 0) {
            writeFile(tempPath, chunk);
            firstTargetChunk = false;
        } else {
            appendFile(tempPath, chunk);
        }
        currentReadOffset += chunk.size();
    }

    // - Suffix (actualEnd to EOF) is copied as-is
    currentReadOffset = actualEnd;
    while (true) {
        std::vector<uint8_t> suffixChunk = readFileBytes(filename, currentReadOffset, CHUNK_SIZE);
        if (suffixChunk.empty()) break;

        appendFile(tempPath, suffixChunk);
        currentReadOffset += suffixChunk.size();
        if (suffixChunk.size() < CHUNK_SIZE) break;
    }

    // 2. Stream the compacted temporary file back to the original file
    currentReadOffset = 0;
    bool firstCleanChunk = true;
    while (true) {
        std::vector<uint8_t> cleanChunk = readFileBytes(tempPath, currentReadOffset, CHUNK_SIZE);
        if (cleanChunk.empty()) break;

        if (firstCleanChunk) {
            writeFile(filename, cleanChunk);
            firstCleanChunk = false;
        } else {
            appendFile(filename, cleanChunk);
        }

        currentReadOffset += cleanChunk.size();
        if (cleanChunk.size() < CHUNK_SIZE) break;
    }

    // 3. Clean up the temporary file by clearing it out
    writeFile(tempPath, std::vector<uint8_t>());
}








// Deletes a specified file from the SPIFFS filesystem
bool FileManager::deleteFile(const std::string& filename) {
    std::string path = getFullPath(filename); // Resolve absolute path string
    if (unlink(path.c_str()) != 0) {          // Execute POSIX unlink syscall to remove file
        ESP_LOGE(TAG, "Failed to delete file: %s", path.c_str());
        return false;
    }
    return true;
}

// Checks whether a file exists without opening an active file handle
bool FileManager::exists(const std::string& filename) {
    std::string path = getFullPath(filename); // Format target path
    struct stat st;
    return stat(path.c_str(), &st) == 0;      // Return true if stat successfully fetches metadata (0 == success)
}

// Retrieves partition capacity and calculates available storage capacity
bool FileManager::getStorageStats(size_t* total_bytes, size_t* free_bytes) {
    if (!m_is_mounted) return false; // Fail fast if filesystem isn't active
    size_t total = 0, used = 0;
    
    // Call ESP-IDF internal driver query function to get partition size info
    if (esp_spiffs_info(m_partition_label.c_str(), &total, &used) == ESP_OK) {
        *total_bytes = total;                                 // Populate total capacity output variable
        *free_bytes = (total > used) ? (total - used) : 0;     // Calculate free memory while preventing integer underflow
        return true;
    }
    return false;
}