// #pragma once                                     // Ensures this header file is only included once during compilation
// #include <string>                                // Includes standard C++ string library for text manipulation
// #include "file_manager.hpp"                      // Includes Layer 2 FileManager so we can read/write raw files

// struct SystemConfig {                            // Defines a clean C++ data structure to hold application settings
//     int audioVolume;                             // Variable storing the audio playback volume level (0-100)
//     int sampleRate;                              // Variable storing the audio sample rate (e.g., 16000 Hz)
//     std::string deviceName;                      // Variable storing the custom network name of the ESP32 board

// };











// class SettingsRepository {                       // Defines the Layer 3 Repository class responsible for data serialization
// public:                                          // Public access specifier for external methods called by Layer 4
//     SettingsRepository(FileManager& fileManager);// Constructor that accepts a reference to our Layer 2 FileManager
//     ~SettingsRepository();                       // Destructor for clean teardown if needed

//     bool load();                                 // Loads settings from storage, parses JSON, and updates the config struct
//     bool save();                                 // Serializes the current config struct into JSON and saves it via FileManager

//     SystemConfig& getConfig();                   // Returns a direct reference to the active config struct for application use
//     void updateConfig(const SystemConfig& newConfig); // Replaces current configuration data with new values

// private:                                         // Private access specifier for internal helper variables and logic
//     FileManager& m_fileManager;                  // Reference link to the underlying Layer 2 FileManager instance
//     SystemConfig m_config;                       // The local in-memory instance holding current system settings
//     const std::string m_filename = "config.json";// Constant string defining the target file name on storage
// };

// #pragma once                                     // Ensures header is only included once
// #include <string>                                // Standard C++ strings
// #include "file_manager.hpp"                      // Layer 2 dependency
// #include <ArduinoJson.h>                         // JSON serialization library

// // TEMPLATE DESIGN: The repository doesn't care what 'T' (the config struct) contains.
// // It only enforces that T must implement serialization/deserialization methods, 
// // or we use a clean visitor/mapping pattern.
// template <typename T>
// class SettingsRepository {
// public:
//     // Constructor binds to Layer 2 storage and takes a default config instance
//     SettingsRepository(FileManager& fileManager, const std::string& filename)
//         : m_fileManager(fileManager), m_filename(filename) {}

//     ~SettingsRepository() = default;

//     // Generic Load: Reads file, parses JSON into a JsonDocument, and passes it to the config object
//     bool load(T& configInstance) {
//         std::string rawData = m_fileManager.readFile(m_filename);
//         if (rawData.empty()) {
//             // File doesn't exist or is empty; caller keeps defaults
//             return false;
//         }

//         JsonDocument doc;
//         DeserializationError error = deserializeJson(doc, rawData);
//         if (error) {
//             return false; // Parsing failed
//         }

//         // Delegate the mapping to the data structure itself (keeping Layer 3 pure)
//         configInstance.deserialize(doc);
//         return true;
//     }

//     // Generic Save: Asks the config instance to populate the JSON document, then serializes it
//     bool save(const T& configInstance) {
//         JsonDocument doc;
        
//         // Delegate serialization to the data structure
//         configInstance.serialize(doc);

//         std::string outputString;
//         serializeJson(doc, outputString);

//         return m_fileManager.writeFile(m_filename, outputString);
//     }

// private:
//     FileManager& m_fileManager;                  // Reference to Layer 2 storage manager
//     std::string m_filename;                      // Target configuration file name
// };