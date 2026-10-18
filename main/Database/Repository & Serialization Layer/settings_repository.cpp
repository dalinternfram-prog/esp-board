// #include "settings_repository.hpp"               // Includes the corresponding header file for this implementation
// #include "esp_log.h"                             // Includes ESP-IDF logging library for terminal print statements
// #include <ArduinoJson.h>                         // Includes the industry-standard ArduinoJson library for parsing JSON

// static const char* TAG = "SettingsRepo";         // Defines a tag label used to filter logs for this repository in the console

// SettingsRepository::SettingsRepository(FileManager& fileManager) 
//     : m_fileManager(fileManager) {               // Initializes the constructor by binding the FileManager reference
//     m_config = {80, 16000, "ESP32_Audio_Node"};  // Sets safe default fallback values in case no saved file exists yet
// }

// SettingsRepository::~SettingsRepository() {}     // Default empty destructor implementation

// bool SettingsRepository::load() {                // Method to read the settings file from storage and parse its contents
//     std::string rawData = m_fileManager.readFile(m_filename); // Calls Layer 2 to read the raw text content of the file
//     if (rawData.empty()) {                       // Checks if the returned file content string is completely empty
//         ESP_LOGW(TAG, "Config file empty or missing. Using defaults."); // Logs a warning message to the console
//         return false;                            // Returns false indicating load operation failed, defaults will be used
//     }

//     JsonDocument doc;                            // Creates a dynamic JSON document container using ArduinoJson v7
//     DeserializationError error = deserializeJson(doc, rawData); // Parses the raw text string into a structured JSON object
//     if (error) {                                 // Checks if the JSON parsing encountered any syntax errors
//         ESP_LOGE(TAG, "Failed to parse JSON config: %s", error.c_str()); // Logs the specific parsing error string
//         return false;                            // Returns false indicating deserialization failure
//     }

//     m_config.audioVolume = doc["volume"] | 80;   // Extracts volume value from JSON, falling back to 80 if key is missing
//     m_config.sampleRate = doc["sample_rate"] | 16000; // Extracts sample rate from JSON, falling back to 16000 if missing
//     m_config.deviceName = doc["device_name"] | "ESP32_Audio_Node"; // Extracts device name string, falling back to default

//     ESP_LOGI(TAG, "Settings loaded successfully from storage."); // Logs a success message to the console
//     return true;                                 // Returns true indicating settings were successfully loaded and parsed
// }

// bool SettingsRepository::save() {                // Method to serialize current struct settings and write them to storage
//     JsonDocument doc;                            // Creates a new empty JSON document container for serialization
//     doc["volume"] = m_config.audioVolume;        // Assigns the current audio volume integer into the JSON object key
//     doc["sample_rate"] = m_config.sampleRate;    // Assigns the current sample rate integer into the JSON object key
//     doc["device_name"] = m_config.deviceName;    // Assigns the current device name string into the JSON object key

//     std::string outputString;                    // Creates a string variable to hold the final serialized JSON text
//     serializeJson(doc, outputString);            // Converts the structured JSON document object into a plain text string

//     bool success = m_fileManager.writeFile(m_filename, outputString); // Calls Layer 2 to write the JSON text string to flash
//     if (success) {                               // Checks if the write operation completed successfully
//         ESP_LOGI(TAG, "Settings saved successfully to storage."); // Logs a success confirmation message
//     } else {                                     // Handles cases where the file write failed
//         ESP_LOGE(TAG, "Failed to write settings file to storage."); // Logs an error message
//     }
//     return success;                              // Returns the boolean success status of the save operation
// }

// SystemConfig& SettingsRepository::getConfig() {  // Method allowing application code to fetch a reference to settings data
//     return m_config;                             // Returns a direct reference to the active in-memory configuration struct
// }

// void SettingsRepository::updateConfig(const SystemConfig& newConfig) { // Method to update local settings in memory
//     m_config = newConfig;                        // Overwrites the existing local config struct with the incoming new values
// }