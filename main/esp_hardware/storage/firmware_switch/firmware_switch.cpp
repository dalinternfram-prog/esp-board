// #include "firmware_switch.hpp" // Include corresponding header file definitions
// #include "esp_log.h"           // Include ESP-IDF logging library header

// FirmwareSwitch::FirmwareSwitch() // Constructor definition initializing member fields
//     : m_running_partition(nullptr), // Initialize running partition pointer to null
//       m_next_partition(nullptr),    // Initialize next partition pointer to null
//       m_is_initialized(false) {}    // Initialize readiness flag to false state

// FirmwareSwitch::~FirmwareSwitch() {} // Destructor definition for clean lifecycle exit

// bool FirmwareSwitch::init() { // Initialization method checking OTA partition states
//     m_running_partition = esp_ota_get_running_partition(); // Fetch partition currently executing application code
//     if (m_running_partition == nullptr) { // Check if running partition query failed
//         ESP_LOGE(TAG, "Failed to get running firmware partition"); // Log error message to console output
//         return false; // Return failure status code
//     }

//     m_next_partition = esp_ota_get_next_update_partition(nullptr); // Fetch next alternate OTA slot partition
//     if (m_next_partition == nullptr) { // Check if alternate partition query failed
//         ESP_LOGE(TAG, "Failed to get next update firmware partition"); // Log error message to console output
//         return false; // Return failure status code
//     }

//     m_is_initialized = true; // Mark component state as ready and initialized
//     ESP_LOGI(TAG, "FirmwareSwitch initialized. Running: [%s], Alternate: [%s]", // Log success info with partition names
//              m_running_partition->label, m_next_partition->label); // Pass partition labels to log macro
//     return true; // Return success status flag
// }

// std::string FirmwareSwitch::getRunningPartitionName() const { // Method returning active partition name string
//     if (!m_is_initialized) return ""; // Return empty string if component uninitialized
//     return std::string(m_running_partition->label); // Return label string of running partition
// }

// std::string FirmwareSwitch::getNextPartitionName() const { // Method returning alternate partition name string
//     if (!m_is_initialized) return ""; // Return empty string if component uninitialized
//     return std::string(m_next_partition->label); // Return label string of alternate partition
// }

// bool FirmwareSwitch::switchToAlternateSlot() { // Method to switch boot target to the alternative slot
//     if (!m_is_initialized) return false; // Verify module readiness state
    
//     ESP_LOGI(TAG, "Switching boot partition to alternate slot: [%s]", m_next_partition->label); // Log switch action
//     esp_err_t ret = esp_ota_set_boot_partition(m_next_partition); // Call ESP-IDF API to set next boot target
//     if (ret != ESP_OK) { // Check if setting boot partition returned an error
//         ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret)); // Log error details
//         return false; // Return failure status
//     }

//     ESP_LOGI(TAG, "Boot partition updated successfully. Restart required to apply."); // Log success notice
//     return true; // Return success status
// }

// bool FirmwareSwitch::rollbackToPrevious() { // Method to handle rollback state actions
//     if (!m_is_initialized) return false; // Verify module readiness state

//     esp_err_t ret = esp_ota_mark_app_invalid_cancel_rollback(); // Cancel rollback or flag current app state
//     if (ret != ESP_OK) { // Check if rollback operation failed
//         ESP_LOGW(TAG, "Rollback state change warning or error: %s", esp_err_to_name(ret)); // Log warning message
//         return false; // Return failure status
//     }

//     ESP_LOGI(TAG, "Rollback configuration updated successfully."); // Log successful action
//     return true; // Return success status
// }


// bool FirmwareSwitch::markAppValid() { // Method to mark current running application as stable and valid
//     if (!m_is_initialized) return false; // Verify module readiness state

//     // Call correct ESP-IDF API to mark app as working well and prevent automatic rollback
//     esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback(); 
//     if (ret != ESP_OK) { // Check if the API call returned an error status
//         ESP_LOGE(TAG, "Failed to mark application as valid: %s", esp_err_to_name(ret)); // Log error description
//         return false; // Return failure status code
//     }

//     ESP_LOGI(TAG, "Application marked as valid and rollback cancelled successfully."); // Log success message
//     return true; // Return success status flag
// }

#include "firmware_switch.hpp" // Include corresponding header file definitions
#include "esp_log.h"           // Include ESP-IDF logging library header

FirmwareSwitch::FirmwareSwitch() // Constructor definition initializing member fields
    : m_running_partition(nullptr), // Initialize running partition pointer to null
      m_next_partition(nullptr),    // Initialize next partition pointer to null
      m_ota_handle(0),              // Initialize OTA write session handle to zero
      m_is_initialized(false),      // Initialize readiness flag to false state
      m_is_updating(false) {}       // Initialize active update flag to false state

FirmwareSwitch::~FirmwareSwitch() { // Destructor definition for clean lifecycle exit
    if (m_is_updating && m_ota_handle != 0) { // Check if an OTA session was left open unintentionally
        esp_ota_end(m_ota_handle);   // Abort/close active OTA session to prevent memory/handle leaks
    }
}

bool FirmwareSwitch::init() { // Initialization method checking OTA partition states
    m_running_partition = esp_ota_get_running_partition(); // Fetch partition currently executing application code
    if (m_running_partition == nullptr) { // Check if running partition query failed
        ESP_LOGE(TAG, "Failed to get running firmware partition"); // Log error message to console output
        return false; // Return failure status code
    }

    m_next_partition = esp_ota_get_next_update_partition(nullptr); // Fetch next alternate OTA slot partition
    if (m_next_partition == nullptr) { // Check if alternate partition query failed
        ESP_LOGE(TAG, "Failed to get next update firmware partition"); // Log error message to console output
        return false; // Return failure status code
    }

    m_is_initialized = true; // Mark component state as ready and initialized
    ESP_LOGI(TAG, "FirmwareSwitch initialized. Running: [%s], Alternate: [%s]", // Log success info with partition names
             m_running_partition->label, m_next_partition->label); // Pass partition labels to log macro
    return true; // Return success status flag
}

std::string FirmwareSwitch::getRunningPartitionName() const { // Method returning active partition name string
    if (!m_is_initialized) return ""; // Return empty string if component uninitialized
    return std::string(m_running_partition->label); // Return label string of running partition
}

std::string FirmwareSwitch::getNextPartitionName() const { // Method returning alternate partition name string
    if (!m_is_initialized) return ""; // Return empty string if component uninitialized
    return std::string(m_next_partition->label); // Return label string of alternate partition
}

bool FirmwareSwitch::switchToAlternateSlot() { // Method to switch boot target to the alternative slot
    if (!m_is_initialized) return false; // Verify module readiness state
    
    ESP_LOGI(TAG, "Switching boot partition to alternate slot: [%s]", m_next_partition->label); // Log switch action
    esp_err_t ret = esp_ota_set_boot_partition(m_next_partition); // Call ESP-IDF API to set next boot target
    if (ret != ESP_OK) { // Check if setting boot partition returned an error
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret)); // Log error details
        return false; // Return failure status
    }

    ESP_LOGI(TAG, "Boot partition updated successfully. Restart required to apply."); // Log success notice
    return true; // Return success status
}

bool FirmwareSwitch::markAppValid() { // Method to mark current running application as stable and valid
    if (!m_is_initialized) return false; // Verify module readiness state

    esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback(); // Call ESP-IDF API to confirm app status
    if (ret != ESP_OK) { // Check if the API call returned an error status
        ESP_LOGE(TAG, "Failed to mark application as valid: %s", esp_err_to_name(ret)); // Log error description
        return false; // Return failure status code
    }

    ESP_LOGI(TAG, "Application marked as valid and rollback cancelled successfully."); // Log success message
    return true; // Return success status flag
}

bool FirmwareSwitch::beginOtaUpdate(size_t image_size) { // Method to start a new OTA update session on alternate slot
    if (!m_is_initialized) return false; // Verify module readiness state
    if (m_is_updating) { // Check if an OTA session is already open
        ESP_LOGW(TAG, "OTA update session is already in progress."); // Log warning notice
        return false; // Return failure status
    }

    ESP_LOGI(TAG, "Beginning OTA update session on alternate partition: [%s]", m_next_partition->label); // Log action
    esp_err_t ret = esp_ota_begin(m_next_partition, image_size, &m_ota_handle); // Begin OTA write operation
    if (ret != ESP_OK) { // Check if starting OTA session failed
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(ret)); // Log error explanation
        return false; // Return failure status
    }

    m_is_updating = true; // Set active update tracking flag to true
    ESP_LOGI(TAG, "OTA begin successful. Ready to receive firmware data chunks."); // Log readiness notice
    return true; // Return success status
}

bool FirmwareSwitch::writeOtaData(const void* data, size_t size) { // Method to write binary firmware data chunks
    if (!m_is_initialized || !m_is_updating) return false; // Fail early if uninitialized or session not active

    esp_err_t ret = esp_ota_write(m_ota_handle, (const void*)data, size); // Write data chunk to flash memory
    if (ret != ESP_OK) { // Check if write operation returned an error
        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(ret)); // Log error details
        return false; // Return failure status
    }

    return true; // Return success status for this chunk
}

bool FirmwareSwitch::endOtaUpdate() { // Method to finalize OTA write session and set boot partition
    if (!m_is_initialized || !m_is_updating) return false; // Verify module and session state

    esp_err_t ret = esp_ota_end(m_ota_handle); // Complete OTA session and verify image integrity/checksum
    m_is_updating = false; // Reset active session flag
    m_ota_handle = 0;      // Clear OTA session handle variable

    if (ret != ESP_OK) { // Check if finalization or image verification failed
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(ret)); // Log error description
        return false; // Return failure status
    }

    ESP_LOGI(TAG, "OTA update written successfully. Setting alternate slot as boot target..."); // Log progress
    return switchToAlternateSlot(); // Automatically set the updated alternate slot as the next boot target
}