  

#include "file_manager.hpp" // Includes the Level 1 header declarations to verify prototype compliance
#include "esp_log.h"        // ESP-IDF logging framework used for console diagnostic and error logs
#include "esp_littlefs.h"   // Official LittleFS API for configuration, mounting, and driver stats
#include "general/io/path.hpp" // Portable path join (general/io/path.hpp)

namespace FileManager { // Opens the FileManager namespace implementation block

// Static tag used by the ESP-IDF logging macros to filter output from this specific module
static const char* TAG = "FileManager"; // Logging identifier shown in console messages

// Stores the base mount path prefix where LittleFS is registered in the Virtual Filesystem (VFS)
static std::string s_base_path = "/littlefs"; // Default mount path prefix stored as internal module state

// Stores the partition label matching the name configured in the partitions.csv partition table
static std::string s_partition_label = "my_files"; // Default label used to locate flash partition

// Tracks the current operational status of the LittleFS filesystem
static bool s_is_mounted = false; // Flag set to true upon successful mount and false upon unmount

std::string getFullPath(const std::string& filename) { // Converts any relative filename to its absolute VFS path
    return general::io::join(s_base_path, filename); // Portable path join (general/io/path.hpp)
} // Closes getFullPath function definition

esp_err_t init(const std::string& base_path, const std::string& partition_label, size_t max_files) { // Mounts LittleFS
    (void)max_files; // LittleFS allocates file descriptors dynamically, argument retained for API compatibility
    s_base_path = base_path; // Overwrites the default base path with caller's requested path
    s_partition_label = partition_label; // Overwrites the default label with caller's partition name

    ESP_LOGI(TAG, "Initializing LittleFS [%s] at [%s]", s_partition_label.c_str(), s_base_path.c_str()); // Logs mount intent

    esp_vfs_littlefs_conf_t conf = { // Defines the configuration structure required by the ESP LittleFS driver
        .base_path = s_base_path.c_str(), // Assigns VFS mount root path pointer
        .partition_label = s_partition_label.c_str(), // Assigns partition table search label pointer
        .format_if_mount_failed = true, // Automatically formats flash partition if corrupt or unformatted
        .dont_mount = false // Requests immediate mounting upon driver registration
    }; // Closes conf structure initialization

    esp_err_t ret = esp_vfs_littlefs_register(&conf); // Requests ESP-IDF VFS to register and mount the driver

    if (ret != ESP_OK) { // Evaluates if the mount attempt encountered an error
        if (ret == ESP_FAIL) { // Mount failure code specifically indicating mounting or formatting issues
            ESP_LOGE(TAG, "Failed to mount or format filesystem"); // Logs mount failure explanation
        } else if (ret == ESP_ERR_NOT_FOUND) { // Partition label lookup failure
            ESP_LOGE(TAG, "Failed to find LittleFS partition '%s'", s_partition_label.c_str()); // Logs missing partition
        } else { // Catches any remaining driver-specific error codes
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret)); // Logs translated error message
        } // Closes error classification ladder
        return ret; // Returns error status code early to caller without modifying state flags
    } // Closes mount check block

    s_is_mounted = true; // Updates internal flag to indicate filesystem is active and ready

    return ESP_OK; // Returns success code indicating clean initialization
} // Closes init function definition

esp_err_t deinit() { // Unmounts and tears down the registered LittleFS instance
    if (!s_is_mounted) { // Checks if filesystem is already unmounted to avoid redundant operations
        return ESP_OK; // Safely returns success if filesystem was not active
    } // Closes unmounted check block

    esp_err_t ret = esp_vfs_littlefs_unregister(s_partition_label.c_str()); // Requests VFS to unregister the partition
    if (ret == ESP_OK) { // Verifies unmount driver call completed successfully
        s_is_mounted = false; // Clears the internal mount state flag
        ESP_LOGI(TAG, "LittleFS unmounted successfully"); // Logs confirmation that filesystem is unloaded
    } else { // Triggered if the driver returns an error on unregister
        ESP_LOGE(TAG, "Failed to unmount LittleFS (%s)", esp_err_to_name(ret)); // Logs unregister error details
    } // Closes unmount status check
    return ret; // Returns the resulting status code back to the caller
} // Closes deinit function definition

bool isMounted() { // Accessor for checking module state externally
    return s_is_mounted; // Returns boolean flag reflecting active mount status
} // Closes isMounted function definition

} // namespace FileManager // Closes the FileManager namespace implementation block