#ifndef NVS_MANAGER_HPP
#define NVS_MANAGER_HPP

#include <string>
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

// Class providing a simplified C++ wrapper around ESP-IDF NVS flash operations
class NvsManager {
public:
    // Constructor initializes internal state and handles default namespace configuration
    NvsManager();

    // Destructor ensures open NVS handles are closed properly upon object destruction
    ~NvsManager();

    // Initializes NVS flash partition and opens the designated namespace handle
    esp_err_t init(const std::string& namespace_name = "storage", const std::string& partition_label = NVS_DEFAULT_PART_NAME);

    // Closes active NVS handles and releases associated hardware resources
    esp_err_t deinit();

    // Writes or updates a key-value string pair in NVS
    bool writeFile(const std::string& key, const std::string& value);

    // Appends text to an existing NVS string entry (reads current value, appends, and rewrites)
    bool appendFile(const std::string& key, const std::string& data);

    // Reads and returns a string value corresponding to the given NVS key
    std::string readFile(const std::string& key);

    // Removes a specific key and its stored value from the open NVS namespace
    bool deleteFile(const std::string& key);

    // Checks whether a given key currently exists within the NVS namespace
    bool exists(const std::string& key);

    // Retrieves total and free entry statistics for the initialized NVS partition
    bool getStorageStats(size_t* total_entries, size_t* free_entries);

private:
    std::string m_namespace_name;   // Stores the target NVS namespace name
    std::string m_partition_label;  // Stores the target NVS partition label
    nvs_handle_t m_nvs_handle;     // Opaque handle for open NVS namespace operations
    bool m_is_initialized;         // Internal boolean tracking initialization status

    // Helper method to sanitize string keys so they comply with NVS length constraints (max 15 chars)
    std::string sanitizeKey(const std::string& key) const;
};

#endif // NVS_MANAGER_HPP