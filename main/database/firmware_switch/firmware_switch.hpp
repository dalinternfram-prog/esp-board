// #ifndef FIRMWARE_SWITCH_HPP // Check if macro is not defined to prevent multiple file inclusions
// #define FIRMWARE_SWITCH_HPP // Define macro to mark this header file as included

// #include "esp_err.h"        // Include ESP-IDF error definitions header
// #include "esp_ota_ops.h"    // Include ESP-IDF Over-The-Air operations header
// #include <string>           // Include standard C++ string library header

// class FirmwareSwitch {      // Declare class managing firmware slot switching and OTA state
// public:                     // Public access specifier for external component interaction
//     FirmwareSwitch();       // Constructor signature for class initialization
//     ~FirmwareSwitch();      // Destructor signature for lifecycle cleanup

//     bool init();            // Initialize OTA switch manager status
//     std::string getRunningPartitionName() const; // Retrieve name of currently running firmware slot
//     std::string getNextPartitionName() const;    // Retrieve name of next target firmware slot
//     bool switchToAlternateSlot();                // Switch active boot partition to the alternate slot
    
// bool markAppValid();                         // Mark current running application as valid and cancel rollback
// private:                    // Private access specifier for internal helper variables and logic
//     const esp_partition_t* m_running_partition; // Pointer structure holding current running partition info
//     const esp_partition_t* m_next_partition;    // Pointer structure holding next alternate partition info
//     bool m_is_initialized;                      // Boolean flag tracking initialization readiness state
//     static constexpr const char* TAG = "FirmwareSwitch"; // Logging tag constant string for console outputs
// };                          // End of FirmwareSwitch class declaration

// #endif                      // End of header guard conditional block





#pragma once
 

#include "esp_err.h"        // Include ESP-IDF error definitions header
#include "esp_ota_ops.h"    // Include ESP-IDF Over-The-Air operations header
#include <string>           // Include standard C++ string library header

class FirmwareSwitch {      // Declare class managing firmware slot switching and OTA state
public:                     // Public access specifier for external component interaction
    FirmwareSwitch();       // Constructor signature for class initialization
    ~FirmwareSwitch();      // Destructor signature for lifecycle cleanup

    bool init();            // Initialize OTA switch manager status
    std::string getRunningPartitionName() const; // Retrieve name of currently running firmware slot
    std::string getNextPartitionName() const;    // Retrieve name of next target firmware slot
    bool switchToAlternateSlot();                // Switch active boot partition to the alternate slot
    bool markAppValid();                         // Mark current running application as valid and cancel rollback

    // Core OTA writing APIs to flash firmware into the alternate slot
    bool beginOtaUpdate(size_t image_size = OTA_SIZE_UNKNOWN); // Initialize an OTA write session on alternate slot
    bool writeOtaData(const void* data, size_t size);          // Write a chunk of binary firmware data to flash
    bool endOtaUpdate();                                       // Finalize OTA session and set boot partition

private:                    // Private access specifier for internal helper variables and logic
    const esp_partition_t* m_running_partition; // Pointer structure holding current running partition info
    const esp_partition_t* m_next_partition;    // Pointer structure holding next alternate partition info
    esp_ota_handle_t m_ota_handle;              // Handle identifier tracking active OTA flash write session
    bool m_is_initialized;                      // Boolean flag tracking initialization readiness state
    bool m_is_updating;                         // Boolean flag tracking active OTA write session state
    static constexpr const char* TAG = "FirmwareSwitch"; // Logging tag constant string for console outputs
};                          // End of FirmwareSwitch class declaration
 