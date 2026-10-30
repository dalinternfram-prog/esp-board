  

// #include "file_manager.hpp"
// #include "esp_log.h"
// #include "esp_spiffs.h"
// #include <sys/unistd.h>
// #include <sys/stat.h>
// #include <string_view>
// #include <algorithm>
// //TODO: we need to add multiple read and write,
// //TODO: open a file frequently is not a good choice,
// //TODO: also need  to return a files size;
// //TODO:i also think there should be a new class for file write and read,so that each file can be tracked independtly,

// namespace FileManager {

// // Tag string used to filter and identify log output originating from this module
// static const char* TAG = "FileManager";

// // Internal state variables initialized with default mount path, partition label, and unmounted status
// static std::string s_base_path = "/spiffs";
// static std::string s_partition_label = "my_files";
// static bool s_is_mounted = false;

// // Constructs a complete VFS path by prepending the base path to relative input names
// static std::string getFullPath(const std::string& filename) {
//     if (filename.empty()) return s_base_path; // Return base path if input filename is empty
//     if (filename[0] == '/') {
//         return s_base_path + filename;         // Concatenate directly if filename includes leading slash
//     }
//     return s_base_path + "/" + filename;       // Inject missing path separator between base and filename
// }

// // Mounts the SPIFFS filesystem and registers it with ESP-IDF's Virtual Filesystem (VFS)
// esp_err_t init(const std::string& base_path, const std::string& partition_label) {
//     s_base_path = base_path;                 // Save custom base path (e.g., "/spiffs")
//     s_partition_label = partition_label; // Save CSV partition label name

//     ESP_LOGI(TAG, "Initializing SPIFFS storage partition [%s] at [%s]", s_partition_label.c_str(), s_base_path.c_str());

//     // Configure the SPIFFS driver parameters
//     esp_vfs_spiffs_conf_t conf = {
//         .base_path = s_base_path.c_str(),
//         .partition_label = s_partition_label.c_str(),
//         .max_files = 5,                      // Maximum concurrent open files allowed
//         .format_if_mount_failed = true         // Auto-formats flash partition on corrupt or unformatted state
//     };

//     // Attempt to register and mount SPIFFS into the Virtual Filesystem layer
//     esp_err_t ret = esp_vfs_spiffs_register(&conf);

//     if (ret != ESP_OK) {
//         if (ret == ESP_FAIL) {
//             ESP_LOGE(TAG, "Failed to mount or format filesystem");
//         } else if (ret == ESP_ERR_NOT_FOUND) {
//             ESP_LOGE(TAG, "Failed to find SPIFFS partition '%s'. Check your partitions.csv!", s_partition_label.c_str());
//         } else {
//             ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
//         }
//         return ret; // Return error code immediately on failure
//     }

//     s_is_mounted = true; // Mark filesystem state as mounted
//     size_t total = 0, free = 0;
    
//     // Log available and total memory stats upon successful mount
//     if (getStorageStats(&total, &free)) {
//         ESP_LOGI(TAG, "SPIFFS Mounted Successfully. Total: %u bytes, Free: %u bytes", total, free);
//     }

//     return ESP_OK; // Initialization complete
// }

// // Safely unregisters the SPIFFS filesystem from the ESP VFS
// esp_err_t deinit() {
//     if (!s_is_mounted) return ESP_OK; // No-op if filesystem isn't currently mounted

//     esp_err_t ret = esp_vfs_spiffs_unregister(s_partition_label.c_str());
//     if (ret == ESP_OK) {
//         s_is_mounted = false; // Reset mount state flag
//         ESP_LOGI(TAG, "SPIFFS unmounted successfully");
//     }
//     return ret;
// }

// bool truncate(const std::string& path, size_t newSize) {
//     std::string fullPath = getFullPath(path);
    
//     // Open for update ("r+") so we don't accidentally wipe it before calling truncation
//     FILE* f = fopen(fullPath.c_str(), "r+");
//     if (!f) return false;
    
//     // Use fileno() to get the underlying file descriptor, then ftruncate
//     int result = ftruncate(fileno(f), newSize);
    
//     fclose(f);
//     return (result == 0);
// }

// bool writeFile(const std::string& filename, const std::string& data, long offset) {
//     std::string path = getFullPath(filename);
    
//     // Open in read/write mode ("r+"), create if it doesn't exist ("w+")
//     FILE* f = fopen(path.c_str(), "r+");
//     if (f == NULL) {
//           ESP_LOGW(TAG, "Failed to open file for r+: %s,(errno: %d),try to use w+", path.c_str(),errno);
//         f = fopen(path.c_str(), "w+");
//         if (f == NULL) {
//             ESP_LOGE(TAG, "Failed to open file for writing: %s,(errno: %d)", path.c_str(),errno);
//             return false;
//         }
//     }
    
//     // 1. Determine the current file size
//     if (fseek(f, 0, SEEK_END) != 0) {
//         ESP_LOGE(TAG, "Failed to seek to end of file");
//         fclose(f);
//         return false;
//     }
//     long fileSize = ftell(f);
//     if (fileSize < 0) {
//         ESP_LOGE(TAG, "Failed to get file size");
//         fclose(f);
//         return false;
//     }
    
//     // If offset is negative, treat it as append mode (write at the end of the file)
//     if (offset < 0) {
//         offset = fileSize;
//     }
    
//     // 2. If offset exceeds file size, explicitly pad the gap with zeros
//     if (offset > fileSize) {
//         long gap = offset - fileSize;
//         constexpr size_t BUF_SIZE = 256;
//         char zeroBuffer[BUF_SIZE] = {0}; // Initialize buffer with zeros
        
//         long remaining = gap;
//         while (remaining > 0) {
//             size_t toWrite = (remaining < (long)BUF_SIZE) ? (size_t)remaining : BUF_SIZE;
//             size_t written = fwrite(zeroBuffer, 1, toWrite, f);
//             if (written != toWrite) {
//                 ESP_LOGE(TAG, "Failed to write zero-padding");
//                 fclose(f);
//                 return false;
//             }
//             remaining -= toWrite;
//         }
//     }
    
//     // 3. Move to the target offset (whether specified, mapped to end, or padded)
//     if (fseek(f, offset, SEEK_SET) != 0) {
//         ESP_LOGE(TAG, "Failed to seek to offset %ld", offset);
//         fclose(f);
//         return false;
//     }
    
//     // 4. Write the actual data
//     size_t written = fwrite(data.c_str(), 1, data.size(), f);
//     fclose(f);

//     // Validate write length
//     if (written != data.size()) {
//         ESP_LOGE(TAG, "Short write: wrote %d of %d bytes", (int)written, (int)data.size());
//         return false;
//     }
//     return true;
// }

// // Overload wrapper for binary data (std::vector<uint8_t>)
// bool writeFile(const std::string& filename, const std::vector<uint8_t>& data, long offset) {
//     // Cast the raw byte pointer to char* and construct a string view / string wrapper safely
//     std::string_view view(reinterpret_cast<const char*>(data.data()), data.size());
//     return writeFile(filename, std::string(view), offset);
// }

// std::vector<uint8_t> readFileBytes(const std::string& filename, long offset, long size) {
//     std::string path = getFullPath(filename); // Convert to complete mount path
//     FILE* f = fopen(path.c_str(), "rb");        // Open file stream for binary reading ("rb")
//     if (f == NULL) {
//         ESP_LOGW(TAG, "Failed to open file for reading: %s,(errno: %d)", path.c_str(), errno);
//         return {}; // Return empty vector if target file cannot be opened
//     }

//     // Check total file size to make sure we don't read past the end
//     if (fseek(f, 0, SEEK_END) != 0) {
//         fclose(f);
//         return {};
//     }
//     long totalLength = ftell(f);
//     if (totalLength < 0 || offset < 0 || offset >= totalLength) {
//         fclose(f);
//         return {}; // Offset is negative or out of bounds
//     }

//     // Determine how many bytes to read
//     size_t bytesToRead = 0;
//     if (size < 0) {
//         // Read until the end of the file from the specified offset
//         bytesToRead = static_cast<size_t>(totalLength - offset);
//     } else {
//         bytesToRead = static_cast<size_t>(size);
//         // Adjust size if requested size exceeds available bytes from offset
//         if (offset + static_cast<long>(bytesToRead) > totalLength) {
//             bytesToRead = static_cast<size_t>(totalLength - offset);
//         }
//     }

//     std::vector<uint8_t> content;
//     if (bytesToRead > 0) {
//         // Seek directly to the target offset from the beginning of the file
//         if (fseek(f, offset, SEEK_SET) != 0) {
//             fclose(f);
//             return {}; // Seek failed
//         }

//         content.resize(bytesToRead);              // Allocate memory ONLY for the requested chunk
//         size_t readBytes = fread(content.data(), 1, bytesToRead, f); // Read bytes into memory
//         if (readBytes != bytesToRead) {
//             content.resize(readBytes); // Trim vector if a short read occurred
//         }
//     }

//     fclose(f); // Free file descriptor resource
//     return content;
// } 

// void shiftBytes(const std::string& filename, size_t shiftDistance, bool left, long long startOffset, long long endOffset) {
//     if (shiftDistance == 0) return;

//     std::string path = getFullPath(filename);
//     FILE* f = fopen(path.c_str(), "rb");
//     if (!f) return;
//     fseek(f, 0, SEEK_END);
//     long fileSize = ftell(f);
//     fclose(f);

//     if (fileSize <= 0) return;

//     long actualStart = (startOffset < 0) ? 0 : static_cast<long>(startOffset);
//     long actualEnd = (endOffset < 0 || endOffset > fileSize) ? fileSize : static_cast<long>(endOffset);

//     if (actualStart >= fileSize || actualStart >= actualEnd) return;

//     const size_t CHUNK_SIZE = 2048;

//     if (left) {
//         // --- SHIFT LEFT ---
//         // Docstring meaning: actualStart is the beginning of the data region to move.
//         // We want to shift this data left by shiftDistance.
//         long currentSrc = actualStart;
//         long currentDst = actualStart - static_cast<long>(shiftDistance);
//         if (currentDst < 0) currentDst = 0; // Safety guard

//         while (currentSrc < actualEnd) {
//             size_t bytesToRead = std::min(CHUNK_SIZE, static_cast<size_t>(actualEnd - currentSrc));
//             std::vector<uint8_t> chunk = readFileBytes(filename, currentSrc, bytesToRead);
//             if (chunk.empty()) break;

//             if (!writeFile(filename, chunk, currentDst)) {
//                 ESP_LOGE(TAG, "Failed to write chunk during left shift");
//                 break;
//             }

//             currentSrc += chunk.size();
//             currentDst += chunk.size();
//         }
//     } 
//     else {
//         // --- SHIFT RIGHT ---
//         // When shifting right, actualStart is the start of the region to push forward.
//         // We iterate backwards from actualEnd to actualStart.
//         long currentSrc = actualEnd;

//         while (currentSrc > actualStart) {
//             size_t bytesToRead = std::min(CHUNK_SIZE, static_cast<size_t>(currentSrc - actualStart));
//             currentSrc -= bytesToRead;

//             std::vector<uint8_t> chunk = readFileBytes(filename, currentSrc, bytesToRead);
//             if (chunk.empty()) break;

//             long targetDst = currentSrc + static_cast<long>(shiftDistance);
//             if (!writeFile(filename, chunk, targetDst)) {
//                 ESP_LOGE(TAG, "Failed to write chunk during right shift");
//                 break;
//             }
//         }
//     }
// } 


// // Deletes a specified file from the SPIFFS filesystem
// bool deleteFile(const std::string& filename) {
//     std::string path = getFullPath(filename); // Resolve absolute path string
//     if (unlink(path.c_str()) != 0) {          // Execute POSIX unlink syscall to remove file
//         ESP_LOGE(TAG, "Failed to delete file: %s", path.c_str());
//         return false;
//     }
//     return true;
// }

// // Checks whether a file exists without opening an active file handle
// bool exists(const std::string& filename) {
//     std::string path = getFullPath(filename); // Format target path
//     struct stat st;
//     return stat(path.c_str(), &st) == 0;      // Return true if stat successfully fetches metadata (0 == success)
// }

// // Retrieves partition capacity and calculates available storage capacity
// bool getStorageStats(size_t* total_bytes, size_t* free_bytes) {
//     if (!s_is_mounted) return false; // Fail fast if filesystem isn't active
//     size_t total = 0, used = 0;
    
//     // Call ESP-IDF internal driver query function to get partition size info
//     if (esp_spiffs_info(s_partition_label.c_str(), &total, &used) == ESP_OK) {
//         *total_bytes = total;                                   // Populate total capacity output variable
//         *free_bytes = (total > used) ? (total - used) : 0;      // Calculate free memory while preventing integer underflow
//         return true;
//     }
//     return false;
// }

// } // namespace FileManager