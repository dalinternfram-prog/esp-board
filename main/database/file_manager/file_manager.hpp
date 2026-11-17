// #pragma once // Ensures this header file is included only once during compilation to avoid redefinition errors

// #include "esp_err.h" // Provides the esp_err_t return type and ESP error code definitions
// #include <string>    // Provides std::string for storing and manipulating file paths
// #include <cstddef>   // Provides the standard size_t data type for byte counting and sizing

// namespace FileManager { // Groups global filesystem operations inside a dedicated namespace to avoid name collisions

// // Mounts and registers the SPIFFS partition to the ESP-IDF Virtual Filesystem (VFS)
// esp_err_t init(const std::string& base_path = "/spiffs",          // The VFS mount point prefix in the virtual file path
//                const std::string& partition_label = "my_files",   // The partition label defined in partitions.csv
//                size_t max_files = 5);                             // The maximum number of concurrently open file handles

// // Safely unmounts and unregisters the SPIFFS filesystem from the VFS layer
// esp_err_t deinit(); // Frees underlying filesystem resources and invalidates the mount state

// // Checks the mount status flag of the filesystem
// bool isMounted(); // Returns true if the filesystem is currently mounted and active, false otherwise

// // Normalizes and prepends the configured base mount path to a relative file path
// std::string getFullPath(const std::string& filename); // Returns the absolute VFS path string (e.g., "/spiffs/data.txt")

// // Queries whether a file exists without opening or locking a file descriptor
// bool exists(const std::string& filename); // Returns true if the file exists on flash, false otherwise

// // Deletes a file from the filesystem using the POSIX unlink interface
// bool deleteFile(const std::string& filename); // Returns true if the file was deleted successfully, false otherwise

// // Retrieves total partition capacity and calculates available free space
// bool getStorageStats(size_t* total_bytes,   // Pointer where the total byte capacity will be written
//                      size_t* free_bytes);   // Pointer where the available unused byte capacity will be written

// } // namespace FileManager // Ends the FileManager namespace declaration














#pragma once // Ensures this header file is included only once during compilation to avoid redefinition errors

#include "esp_err.h" // Provides the esp_err_t return type and ESP error code definitions
#include <string>    // Provides std::string for storing and manipulating file paths
#include <cstddef>   // Provides the standard size_t data type for byte counting and sizing

namespace FileManager { // Groups global filesystem operations inside a dedicated namespace to avoid name collisions

// Mounts and registers the LittleFS partition to the ESP-IDF Virtual Filesystem (VFS)
esp_err_t init(const std::string& base_path = "/littlefs",        // The VFS mount point prefix in the virtual file path
               const std::string& partition_label = "my_files",   // The partition label defined in partitions.csv
               size_t max_files = 5);                             // Kept for API parity; LittleFS allocates handles dynamically

// Safely unmounts and unregisters the LittleFS filesystem from the VFS layer
esp_err_t deinit(); // Frees underlying filesystem resources and invalidates the mount state

// Checks the mount status flag of the filesystem
bool isMounted(); // Returns true if the filesystem is currently mounted and active, false otherwise

// Normalizes and prepends the configured base mount path to a relative file path
std::string getFullPath(const std::string& filename); // Returns the absolute VFS path string (e.g., "/littlefs/data.txt")

// Queries whether a file exists without opening or locking a file descriptor
bool exists(const std::string& filename); // Returns true if the file exists on flash, false otherwise

// Deletes a file from the filesystem using the POSIX unlink interface
bool deleteFile(const std::string& filename); // Returns true if the file was deleted successfully, false otherwise

// Retrieves total partition capacity and calculates available free space
bool getStorageStats(size_t* total_bytes,   // Pointer where the total byte capacity will be written
                     size_t* free_bytes);   // Pointer where the available unused byte capacity will be written

} // namespace FileManager // Ends the FileManager namespace declaration