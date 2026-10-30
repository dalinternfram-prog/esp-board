#include "file_manager.hpp" // Includes the Level 1 header declarations to verify prototype compliance
#include "esp_log.h"        // ESP-IDF logging framework used for console diagnostic and error logs
#include "esp_spiffs.h"     // Official ESP-IDF SPIFFS API for configuration, mounting, and driver stats
#include <sys/unistd.h>     // POSIX standard header containing unlink() used for file deletion
#include <sys/stat.h>       // POSIX file system status header containing stat() and struct stat
#include <cerrno>           // Defines errno macro for tracking system-level errors from failed calls

namespace FileManager { // Opens the FileManager namespace implementation block

// Static tag used by the ESP-IDF logging macros to filter output from this specific module
static const char* TAG = "FileManager"; // Logging identifier shown in console messages

// Stores the base mount path prefix where SPIFFS is registered in the Virtual Filesystem (VFS)
static std::string s_base_path = "/spiffs"; // Default mount path prefix stored as internal module state

// Stores the partition label matching the name configured in the partitions.csv partition table
static std::string s_partition_label = "my_files"; // Default label used to locate flash partition

// Tracks the current operational status of the SPIFFS filesystem
static bool s_is_mounted = false; // Flag set to true upon successful mount and false upon unmount

std::string getFullPath(const std::string& filename) { // Converts any relative filename to its absolute VFS path
    if (filename.empty()) { // Checks if an empty string argument was passed
        return s_base_path; // Returns the base mount path directly if no filename was supplied
    } // Closes empty filename check block
    if (filename[0] == '/') { // Checks if the caller already provided a leading directory slash
        return s_base_path + filename; // Concatenates base path directly to the supplied string
    } // Closes leading slash check block
    return s_base_path + "/" + filename; // Injects the missing slash separator between base and filename
} // Closes getFullPath function definition

esp_err_t init(const std::string& base_path, const std::string& partition_label, size_t max_files) { // Mounts SPIFFS
    s_base_path = base_path; // Overwrites the default base path with caller's requested path
    s_partition_label = partition_label; // Overwrites the default label with caller's partition name

    ESP_LOGI(TAG, "Initializing SPIFFS [%s] at [%s]", s_partition_label.c_str(), s_base_path.c_str()); // Logs mount intent

    esp_vfs_spiffs_conf_t conf = { // Defines the configuration structure required by the ESP SPIFFS driver
        .base_path = s_base_path.c_str(), // Assigns VFS mount root path pointer
        .partition_label = s_partition_label.c_str(), // Assigns partition table search label pointer
        .max_files = max_files, // Sets maximum concurrent open file descriptors allowed
        .format_if_mount_failed = true // Automatically formats flash partition if corrupt or unformatted
    }; // Closes conf structure initialization

    esp_err_t ret = esp_vfs_spiffs_register(&conf); // Requests ESP-IDF VFS to register and mount the driver

    if (ret != ESP_OK) { // Evaluates if the mount attempt encountered an error
        if (ret == ESP_FAIL) { // Mount failure code specifically indicating mounting or formatting issues
            ESP_LOGE(TAG, "Failed to mount or format filesystem"); // Logs mount failure explanation
        } else if (ret == ESP_ERR_NOT_FOUND) { // Partition label lookup failure
            ESP_LOGE(TAG, "Failed to find SPIFFS partition '%s'", s_partition_label.c_str()); // Logs missing partition
        } else { // Catches any remaining driver-specific error codes
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret)); // Logs translated error message
        } // Closes error classification ladder
        return ret; // Returns error status code early to caller without modifying state flags
    } // Closes mount check block

    s_is_mounted = true; // Updates internal flag to indicate filesystem is active and ready
    size_t total = 0, free = 0; // Stack variables to hold capacity diagnostics

    if (getStorageStats(&total, &free)) { // Attempts to query partition capacity immediately after mount
        ESP_LOGI(TAG, "Mounted. Total: %u bytes, Free: %u bytes", total, free); // Logs total and free memory on success
    } // Closes memory reporting check

    return ESP_OK; // Returns success code indicating clean initialization
} // Closes init function definition

esp_err_t deinit() { // Unmounts and tears down the registered SPIFFS instance
    if (!s_is_mounted) { // Checks if filesystem is already unmounted to avoid redundant operations
        return ESP_OK; // Safely returns success if filesystem was not active
    } // Closes unmounted check block

    esp_err_t ret = esp_vfs_spiffs_unregister(s_partition_label.c_str()); // Requests VFS to unregister the partition
    if (ret == ESP_OK) { // Verifies unmount driver call completed successfully
        s_is_mounted = false; // Clears the internal mount state flag
        ESP_LOGI(TAG, "SPIFFS unmounted successfully"); // Logs confirmation that filesystem is unloaded
    } else { // Triggered if the driver returns an error on unregister
        ESP_LOGE(TAG, "Failed to unmount SPIFFS (%s)", esp_err_to_name(ret)); // Logs unregister error details
    } // Closes unmount status check
    return ret; // Returns the resulting status code back to the caller
} // Closes deinit function definition

bool isMounted() { // Accessor for checking module state externally
    return s_is_mounted; // Returns boolean flag reflecting active mount status
} // Closes isMounted function definition

bool exists(const std::string& filename) { // Determines if a file is present on storage
    std::string path = getFullPath(filename); // Obtains fully qualified VFS path including base mount directory
    struct stat st; // Creates POSIX stat structure to receive file metadata
    return (stat(path.c_str(), &st) == 0); // Returns true if stat succeeds (code 0), false if not found or inaccessible
} // Closes exists function definition

bool deleteFile(const std::string& filename) { // Removes a targeted file from storage
    std::string path = getFullPath(filename); // Resolves relative filename into full VFS filesystem path
    if (unlink(path.c_str()) != 0) { // Invokes POSIX unlink syscall; returns 0 on success, -1 on failure
        ESP_LOGE(TAG, "Failed to delete file '%s' (errno: %d)", path.c_str(), errno); // Logs failure along with errno
        return false; // Returns false to inform caller that deletion failed
    } // Closes deletion check block
    return true; // Returns true indicating successful deletion from flash
} // Closes deleteFile function definition

bool getStorageStats(size_t* total_bytes, size_t* free_bytes) { // Queries total and available flash partition space
    if (!s_is_mounted) { // Verifies filesystem is mounted before issuing driver query
        return false; // Returns failure immediately if the filesystem is currently offline
    } // Closes mount check block

    size_t total = 0; // Local variable to receive total byte capacity from ESP driver
    size_t used = 0;  // Local variable to receive used byte count from ESP driver

    if (esp_spiffs_info(s_partition_label.c_str(), &total, &used) == ESP_OK) { // Driver query function to extract metrics
        if (total_bytes != nullptr) { // Verifies caller provided a valid non-null output pointer for total bytes
            *total_bytes = total; // Assigns total capacity to output pointer destination
        } // Closes total_bytes null check
        if (free_bytes != nullptr) { // Verifies caller provided a valid non-null output pointer for free bytes
            *free_bytes = (total > used) ? (total - used) : 0; // Calculates free space and guards against underflow
        } // Closes free_bytes null check
        return true; // Returns true indicating stats were successfully queried and populated
    } // Closes driver query evaluation block

    return false; // Returns false if the ESP-IDF driver call failed to fetch stats
} // Closes getStorageStats function definition

} // namespace FileManager // Closes the FileManager namespace implementation block