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
 bool FileManager::truncate(const std::string& path, size_t newSize) {
    std::string fullPath = getFullPath(path);
    
    // Open for update ("r+") so we don't accidentally wipe it before calling truncation
    FILE* f = fopen(fullPath.c_str(), "r+");
    if (!f) return false;
    
    // Use fileno() to get the underlying file descriptor, then ftruncate
    int result = ftruncate(fileno(f), newSize);
    
    fclose(f);
    return (result == 0);
}

 bool FileManager::writeFile(const std::string& filename, const std::string& data, long offset) {
    std::string path = getFullPath(filename);
    
    // Open in read/write mode ("r+"), create if it doesn't exist ("w+")
    FILE* f = fopen(path.c_str(), "r+");
    if (f == NULL) {
        f = fopen(path.c_str(), "w+");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing: %s", path.c_str());
            return false;
        }
    }
    
    // 1. Determine the current file size
    if (fseek(f, 0, SEEK_END) != 0) {
        ESP_LOGE(TAG, "Failed to seek to end of file");
        fclose(f);
        return false;
    }
    long fileSize = ftell(f);
    if (fileSize < 0) {
        ESP_LOGE(TAG, "Failed to get file size");
        fclose(f);
        return false;
    }
    
    // If offset is negative, treat it as append mode (write at the end of the file)
    if (offset < 0) {
        offset = fileSize;
    }
    
    // 2. If offset exceeds file size, explicitly pad the gap with zeros
    if (offset > fileSize) {
        long gap = offset - fileSize;
        constexpr size_t BUF_SIZE = 256;
        char zeroBuffer[BUF_SIZE] = {0}; // Initialize buffer with zeros
        
        long remaining = gap;
        while (remaining > 0) {
            size_t toWrite = (remaining < (long)BUF_SIZE) ? (size_t)remaining : BUF_SIZE;
            size_t written = fwrite(zeroBuffer, 1, toWrite, f);
            if (written != toWrite) {
                ESP_LOGE(TAG, "Failed to write zero-padding");
                fclose(f);
                return false;
            }
            remaining -= toWrite;
        }
    }
    
    // 3. Move to the target offset (whether specified, mapped to end, or padded)
    if (fseek(f, offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Failed to seek to offset %ld", offset);
        fclose(f);
        return false;
    }
    
    // 4. Write the actual data
    size_t written = fwrite(data.c_str(), 1, data.size(), f);
    fclose(f);

    // Validate write length
    if (written != data.size()) {
        ESP_LOGE(TAG, "Short write: wrote %d of %d bytes", (int)written, (int)data.size());
        return false;
    }
    return true;
}
// Overload wrapper for binary data (std::vector<uint8_t>)
bool FileManager::writeFile(const std::string& filename, const std::vector<uint8_t>& data, long offset) {
    // Cast the raw byte pointer to char* and construct a string view / string wrapper safely
    std::string_view view(reinterpret_cast<const char*>(data.data()), data.size());
    return writeFile(filename, std::string(view), offset);
}

std::vector<uint8_t> FileManager::readFileBytes(const std::string& filename, long offset, long size) {
    std::string path = getFullPath(filename); // Convert to complete mount path
    FILE* f = fopen(path.c_str(), "rb");        // Open file stream for binary reading ("rb")
    if (f == NULL) {
        ESP_LOGW(TAG, "Failed to open file for reading: %s", path.c_str());
        return {}; // Return empty vector if target file cannot be opened
    }

    // Check total file size to make sure we don't read past the end
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return {};
    }
    long totalLength = ftell(f);
    if (totalLength < 0 || offset < 0 || offset >= totalLength) {
        fclose(f);
        return {}; // Offset is negative or out of bounds
    }

    // Determine how many bytes to read
    size_t bytesToRead = 0;
    if (size < 0) {
        // Read until the end of the file from the specified offset
        bytesToRead = static_cast<size_t>(totalLength - offset);
    } else {
        bytesToRead = static_cast<size_t>(size);
        // Adjust size if requested size exceeds available bytes from offset
        if (offset + static_cast<long>(bytesToRead) > totalLength) {
            bytesToRead = static_cast<size_t>(totalLength - offset);
        }
    }

    std::vector<uint8_t> content;
    if (bytesToRead > 0) {
        // Seek directly to the target offset from the beginning of the file
        if (fseek(f, offset, SEEK_SET) != 0) {
            fclose(f);
            return {}; // Seek failed
        }

        content.resize(bytesToRead);              // Allocate memory ONLY for the requested chunk
        size_t readBytes = fread(content.data(), 1, bytesToRead, f); // Read bytes into memory
        if (readBytes != bytesToRead) {
            content.resize(readBytes); // Trim vector if a short read occurred
        }
    }

    fclose(f); // Free file descriptor resource
    return content;
}


// Method to shift bytes within a file by a given shiftDistance (left or right)
// within an optional range [startOffset, endOffset]. Pass negative values (-1) to use defaults.
void FileManager::shiftBytes(const std::string& filename, size_t shiftDistance, bool left, long long startOffset, long long endOffset) {
    if (shiftDistance == 0) return;

    // Determine total file size using readFileBytes or a helper
    // (Opening and checking size via a quick read or fseek internally)
    std::string path = getFullPath(filename);
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for shifting: %s", path.c_str());
        return;
    }
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fclose(f);

    if (fileSize <= 0) return;

    // Apply rules: if offset < 0, fallback to defaults (0 for start, fileSize for end)
    long actualStart = (startOffset < 0) ? 0 : static_cast<long>(startOffset);
    long actualEnd = (endOffset < 0 || endOffset > fileSize) ? fileSize : static_cast<long>(endOffset);

    if (actualStart >= fileSize || actualStart >= actualEnd) return;

    const size_t CHUNK_SIZE = 2048; // Fixed 2KB chunk size to protect ESP32 RAM

    if (left) {
        // --- SHIFT LEFT (Forward iteration: left-to-right) ---
        // Move data from (actualStart + shiftDistance) down to actualStart
        long srcStart = actualStart + static_cast<long>(shiftDistance);
        if (srcStart > actualEnd) srcStart = actualEnd;

        long currentSrc = srcStart;
        long currentDst = actualStart;

        while (currentSrc < actualEnd) {
            size_t bytesToRead = std::min(CHUNK_SIZE, static_cast<size_t>(actualEnd - currentSrc));
            std::vector<uint8_t> chunk = readFileBytes(filename, currentSrc, bytesToRead);
            if (chunk.empty()) break;

            if (!writeFile(filename, chunk, currentDst)) {
                ESP_LOGE(TAG, "Failed to write chunk during left shift");
                break;
            }

            currentSrc += chunk.size();
            currentDst += chunk.size();
        }
    } else {
        // --- SHIFT RIGHT (Backward iteration: right-to-left) ---
        // Move data from actualEnd back to actualStart, shifted right.
        // We iterate backwards to prevent overwriting unread data.
        long currentSrc = actualEnd;

        while (currentSrc > actualStart) {
            size_t bytesToRead = std::min(CHUNK_SIZE, static_cast<size_t>(currentSrc - actualStart));
            currentSrc -= bytesToRead;

            std::vector<uint8_t> chunk = readFileBytes(filename, currentSrc, bytesToRead);
            if (chunk.empty()) break;

            long targetDst = currentSrc + static_cast<long>(shiftDistance);
            // Our updated writeFile automatically handles zero-padding if targetDst exceeds fileSize
            if (!writeFile(filename, chunk, targetDst)) {
                ESP_LOGE(TAG, "Failed to write chunk during right shift");
                break;
            }
        }
    }
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