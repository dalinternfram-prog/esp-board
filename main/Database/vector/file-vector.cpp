
// #include "file-vector.hpp"
// #include "esp_timer.h"
// #include <cstddef>
// #include <vector>

// namespace file {

// // Helper method to read a text file (like JSON metadata) using readFileBytes
// std::string fileVector::readTextFile(const std::string& path) {
//     if (!FileManager_) return "";
//     std::vector<uint8_t> bytes = FileManager_->readFileBytes(path, 0, -1);
//     return std::string(bytes.begin(), bytes.end());
// }

// // Constructor implementation
// fileVector::fileVector(std::string fileName, std::shared_ptr<FileManager> FileManager_)
//     : FileManager_(std::move(FileManager_)), fileName(std::move(fileName)) {
    
//     std::string jsonPath = fileName + ".json";
//     if (!FileManager_->exists(jsonPath)) {
//         JsonDocument doc;
//         doc["elements"] = JsonArray();
        
//         std::string output;
//         serializeJson(doc, output);
//         FileManager_->writeFile(jsonPath, output, 0);
//     }
// }

// // Method to append raw byte data (using writeFile with offset = -1 for append mode)
// void fileVector::push_back(const std::vector<uint8_t>& data) {
//     if (!FileManager_) return;

//     std::string jsonPath = fileName + ".json";
//     std::string jsonStr = readTextFile(jsonPath);

//     JsonDocument doc;
//     DeserializationError error = deserializeJson(doc, jsonStr);
    
//     if (error) {
//         doc.clear();
//         doc["elements"] = JsonArray();
//     }

//     JsonArray elements = doc["elements"].to<JsonArray>();

//     int currentOffset = 0;
//     if (elements.size() > 0) {
//         // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
//         JsonObject lastElement = elements[elements.size() - 1];
//         currentOffset = lastElement["offset"].as<int>() + lastElement["byteSize"].as<size_t>();
//     }

//     // Use writeFile with offset < 0 (-1) for append mode
//     FileManager_->writeFile(fileName, data, -1);

//     JsonObject newElement = elements.add<JsonObject>();
//     newElement["byteSize"] = data.size();
//     newElement["offset"] = currentOffset;
//     newElement["AddingTime"] = static_cast<uint32_t>(esp_timer_get_time() / 1000);

//     std::string updatedJson;
//     serializeJson(doc, updatedJson);
//     FileManager_->writeFile(jsonPath, updatedJson, 0);
// }

// // Method to retrieve the current size (number of elements)
// size_t fileVector::GetCurrentSize() const {
//     if (!FileManager_) return 0;
//     std::string jsonPath = fileName + ".json";
//     std::vector<uint8_t> bytes = FileManager_->readFileBytes(jsonPath, 0, -1);
//     std::string jsonStr(bytes.begin(), bytes.end());
    
//     JsonDocument doc;
//     if (deserializeJson(doc, jsonStr) == DeserializationError::Ok) {
//         JsonArray elements = doc["elements"].as<JsonArray>();
//         return elements.size();
//     }
//     return 0;
// }

// // Method to fetch a specific element's raw byte data by index
// std::vector<uint8_t> fileVector::Fetch(int i) {
//     std::vector<uint8_t> emptyResult;
//     if (!FileManager_) return emptyResult;

//     std::string jsonPath = fileName + ".json";
//     std::string jsonStr = readTextFile(jsonPath);

//     JsonDocument doc;
//     if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
//         return emptyResult;
//     }

//     JsonArray elements = doc["elements"].as<JsonArray>();
//     if (i < 0 || i >= static_cast<int>(elements.size())) {
//         return emptyResult;
//     }

//     JsonObject target = elements[i];
//     size_t byteSize = target["byteSize"].as<size_t>();
//     size_t offset = target["offset"].as<size_t>();

//     return FileManager_->readFileBytes(fileName, offset, byteSize);
// }

 
// // Generalized Pop method: i = -1 (or last index) pops the last element, 0 pops the first, or any index
// std::vector<uint8_t> fileVector::Pop(int i) {
//     std::vector<uint8_t> emptyResult;
//     if (!FileManager_) return emptyResult;

//     std::string jsonPath = fileName + ".json";
//     std::string jsonStr = readTextFile(jsonPath);

//     JsonDocument doc;
//     if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
//         return emptyResult;
//     }

//     JsonArray elements = doc["elements"].as<JsonArray>();
//     size_t n = elements.size();
//     if (n == 0) {
//         return emptyResult;
//     }

//     // Convert negative index (e.g., -1) to positive index
//     if (i < 0) {
//         i = static_cast<int>(n) + i;
//     }

//     // Validate index bounds
//     if (i < 0 || i >= static_cast<int>(n)) {
//         return emptyResult;
//     }

//     JsonObject target = elements[i];
//     size_t byteSize = target["byteSize"].as<size_t>();
//     size_t offset = target["offset"].as<size_t>();

//     // 1. Read target element bytes before removal
//     std::vector<uint8_t> result = FileManager_->readFileBytes(fileName, offset, byteSize);

//     // 2. Remove element reference from JSON array
//     elements.remove(i);

//     // 3. Handle data shifting / truncation based on pop location
//     // FIX: Replaced elements.empty() with elements.size() == 0 for ArduinoJson v7
//     if (elements.size() == 0) {
//         // Queue is completely empty: wipe file clean instantly using truncation (or write empty)
//         FileManager_->truncate(fileName, 0);
//     } 
//     else if (i == static_cast<int>(n) - 1) {
//         // Popping the LAST element: lightweight, just truncate the file to the new end size
//         // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
//         size_t newFileSizeBytes = elements[elements.size() - 1]["offset"].as<size_t>() + elements[elements.size() - 1]["byteSize"].as<size_t>();
//         FileManager_->truncate(fileName, newFileSizeBytes);
//     } 
//     else if (i == 0) {
//         // Popping the FRONT element: Apply LAZY COMPACTION strategy!
//         const size_t COMPACTION_THRESHOLD = 4096; // 4KB threshold before shifting physical bytes
//         size_t newStartOffset = elements[0]["offset"].as<size_t>();

//         if (newStartOffset >= COMPACTION_THRESHOLD) {
//             // Shift bytes left from newStartOffset up to EOF (-1) by shiftDistance = newStartOffset
//             FileManager_->shiftBytes(fileName, newStartOffset, true, newStartOffset, -1);

//             // Truncate the trailing dead space off the end of the file
//             // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
//             size_t newFileSizeBytes = elements[elements.size() - 1]["offset"].as<size_t>() + elements[elements.size() - 1]["byteSize"].as<size_t>() - newStartOffset;
//             FileManager_->truncate(fileName, newFileSizeBytes);

//             // Shift all remaining JSON element offsets back down to start at 0
//             for (JsonObject el : elements) {
//                 size_t currentElOffset = el["offset"].as<size_t>();
//                 el["offset"] = currentElOffset - newStartOffset;
//             }
//         }
//     } 
//     else {
//         // Popping from the MIDDLE: Shift subsequent bytes left by byteSize starting right after the removed element
//         size_t shiftStart = offset + byteSize;
//         FileManager_->shiftBytes(fileName, byteSize, true, shiftStart, -1);

//         // Adjust all subsequent JSON element offsets down
//         for (size_t j = i; j < elements.size(); ++j) {
//             JsonObject el = elements[j];
//             size_t currentElOffset = el["offset"].as<size_t>();
//             el["offset"] = currentElOffset - byteSize;
//         }

//         // Truncate the file at the new valid end size
//         // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
//         size_t newFileSizeBytes = elements[elements.size() - 1]["offset"].as<size_t>() + elements[elements.size() - 1]["byteSize"].as<size_t>();
//         FileManager_->truncate(fileName, newFileSizeBytes);
//     }

//     // 4. Save updated JSON metadata
//     std::string updatedJson;
//     serializeJson(doc, updatedJson);
//     FileManager_->writeFile(jsonPath, updatedJson, 0);

//     return result;
// }


//  // Method to modify an existing element's raw byte data by index
// bool fileVector::modify(const std::vector<uint8_t>& data, int i) {
//     // i == -1 means the last one;
//     // if size is not consistent, error, return false;
//     if (!FileManager_) return false;

//     std::string jsonPath = fileName + ".json";
//     std::string jsonStr = readTextFile(jsonPath);

//     JsonDocument doc;
//     if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
//         return false;
//     }

//     JsonArray elements = doc["elements"].as<JsonArray>();
//     size_t n = elements.size();
//     if (n == 0) {
//         return false;
//     }

//     // Convert negative index (e.g., -1) to positive index
//     if (i < 0) {
//         i = static_cast<int>(n) + i;
//     }

//     // Validate index bounds
//     if (i < 0 || i >= static_cast<int>(n)) {
//         return false;
//     }

//     JsonObject target = elements[i];
//     size_t byteSize = target["byteSize"].as<size_t>();

//     // Check if the new data size matches the existing element's byte size
//     if (data.size() != byteSize) {
//         return false;
//     }

//     size_t offset = target["offset"].as<size_t>();

//     // 1. Write the new byte data into the binary file at the exact offset
//     FileManager_->writeFile(fileName, data, offset);

//     // 2. Optionally update the timestamp to reflect when it was modified
//     target["AddingTime"] = static_cast<uint32_t>(esp_timer_get_time() / 1000);

//     // 3. Save updated JSON metadata
//     std::string updatedJson;
//     serializeJson(doc, updatedJson);
//     FileManager_->writeFile(jsonPath, updatedJson, 0);

//     return true;
// }


// } // namespace file





























#include "file-vector.hpp"
#include "esp_timer.h"
#include <cstddef>
#include <vector>

namespace file {

// Helper method to read a text file (like JSON metadata) using readFileBytes
std::string fileVector::readTextFile(const std::string& path)const {
    std::vector<uint8_t> bytes = FileManager::readFileBytes(path, 0, -1);
    return std::string(bytes.begin(), bytes.end());
}

// Constructor implementation
fileVector::fileVector(std::string fileName)
    : fileName(std::move(fileName)) {
    
    std::string jsonPath = fileName + ".json";
    if (!FileManager::exists(jsonPath)) {
        JsonDocument doc;
        doc["elements"] = JsonArray();
        
        std::string output;
        serializeJson(doc, output);
        FileManager::writeFile(jsonPath, output, 0);
    }
}

// Method to append raw byte data (using writeFile with offset = -1 for append mode)
void fileVector::push_back(const std::vector<uint8_t>& data) {
    std::string jsonPath = fileName + ".json";
    std::string jsonStr = readTextFile(jsonPath);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        doc.clear();
        doc["elements"] = JsonArray();
    }

    JsonArray elements = doc["elements"].to<JsonArray>();

    int currentOffset = 0;
    if (elements.size() > 0) {
        // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
        JsonObject lastElement = elements[elements.size() - 1];
        currentOffset = lastElement["offset"].as<int>() + lastElement["byteSize"].as<size_t>();
    }

    // Use writeFile with offset < 0 (-1) for append mode
    FileManager::writeFile(fileName, data, -1);

    JsonObject newElement = elements.add<JsonObject>();
    newElement["byteSize"] = data.size();
    newElement["offset"] = currentOffset;
    newElement["AddingTime"] = static_cast<uint32_t>(esp_timer_get_time() / 1000);

    std::string updatedJson;
    serializeJson(doc, updatedJson);
    FileManager::writeFile(jsonPath, updatedJson, 0);
}

// Method to retrieve the current size (number of elements)
size_t fileVector::GetCurrentSize() const {
    std::string jsonPath = fileName + ".json";
    std::vector<uint8_t> bytes = FileManager::readFileBytes(jsonPath, 0, -1);
    std::string jsonStr(bytes.begin(), bytes.end());
    
    JsonDocument doc;
    if (deserializeJson(doc, jsonStr) == DeserializationError::Ok) {
        JsonArray elements = doc["elements"].as<JsonArray>();
        return elements.size();
    }
    return 0;
}

// Method to fetch a specific element's raw byte data by index
std::vector<uint8_t> fileVector::Fetch(int i)const {
    std::vector<uint8_t> emptyResult;

    std::string jsonPath = fileName + ".json";
    std::string jsonStr = readTextFile(jsonPath);

    JsonDocument doc;
    if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
        return emptyResult;
    }

    JsonArray elements = doc["elements"].as<JsonArray>();
    if (i < 0 || i >= static_cast<int>(elements.size())) {
        return emptyResult;
    }

    JsonObject target = elements[i];
    size_t byteSize = target["byteSize"].as<size_t>();
    size_t offset = target["offset"].as<size_t>();

    return FileManager::readFileBytes(fileName, offset, byteSize);
}

 
// Generalized Pop method: i = -1 (or last index) pops the last element, 0 pops the first, or any index
std::vector<uint8_t> fileVector::Pop(int i) {
    std::vector<uint8_t> emptyResult;

    std::string jsonPath = fileName + ".json";
    std::string jsonStr = readTextFile(jsonPath);

    JsonDocument doc;
    if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
        return emptyResult;
    }

    JsonArray elements = doc["elements"].as<JsonArray>();
    size_t n = elements.size();
    if (n == 0) {
        return emptyResult;
    }

    // Convert negative index (e.g., -1) to positive index
    if (i < 0) {
        i = static_cast<int>(n) + i;
    }

    // Validate index bounds
    if (i < 0 || i >= static_cast<int>(n)) {
        return emptyResult;
    }

    JsonObject target = elements[i];
    size_t byteSize = target["byteSize"].as<size_t>();
    size_t offset = target["offset"].as<size_t>();

    // 1. Read target element bytes before removal
    std::vector<uint8_t> result = FileManager::readFileBytes(fileName, offset, byteSize);

    // 2. Remove element reference from JSON array
    elements.remove(i);

    // 3. Handle data shifting / truncation based on pop location
    // FIX: Replaced elements.empty() with elements.size() == 0 for ArduinoJson v7
    if (elements.size() == 0) {
        // Queue is completely empty: wipe file clean instantly using truncation (or write empty)
        FileManager::truncate(fileName, 0);
    } 
    else if (i == static_cast<int>(n) - 1) {
        // Popping the LAST element: lightweight, just truncate the file to the new end size
        // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
        size_t newFileSizeBytes = elements[elements.size() - 1]["offset"].as<size_t>() + elements[elements.size() - 1]["byteSize"].as<size_t>();
        FileManager::truncate(fileName, newFileSizeBytes);
    } 
    else if (i == 0) {
        // Popping the FRONT element: Apply LAZY COMPACTION strategy!
        const size_t COMPACTION_THRESHOLD = 4096; // 4KB threshold before shifting physical bytes
        size_t newStartOffset = elements[0]["offset"].as<size_t>();

        if (newStartOffset >= COMPACTION_THRESHOLD) {
            // Shift bytes left from newStartOffset up to EOF (-1) by shiftDistance = newStartOffset
            FileManager::shiftBytes(fileName, newStartOffset, true, newStartOffset, -1);

            // Truncate the trailing dead space off the end of the file
            // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
            size_t newFileSizeBytes = elements[elements.size() - 1]["offset"].as<size_t>() + elements[elements.size() - 1]["byteSize"].as<size_t>() - newStartOffset;
            FileManager::truncate(fileName, newFileSizeBytes);

            // Shift all remaining JSON element offsets back down to start at 0
            for (JsonObject el : elements) {
                size_t currentElOffset = el["offset"].as<size_t>();
                el["offset"] = currentElOffset - newStartOffset;
            }
        }
    } 
    else {
        // Popping from the MIDDLE: Shift subsequent bytes left by byteSize starting right after the removed element
        size_t shiftStart = offset + byteSize;
        FileManager::shiftBytes(fileName, byteSize, true, shiftStart, -1);

        // Adjust all subsequent JSON element offsets down
        for (size_t j = i; j < elements.size(); ++j) {
            JsonObject el = elements[j];
            size_t currentElOffset = el["offset"].as<size_t>();
            el["offset"] = currentElOffset - byteSize;
        }

        // Truncate the file at the new valid end size
        // FIX: Replaced elements.back() with elements[elements.size() - 1] for ArduinoJson v7
        size_t newFileSizeBytes = elements[elements.size() - 1]["offset"].as<size_t>() + elements[elements.size() - 1]["byteSize"].as<size_t>();
        FileManager::truncate(fileName, newFileSizeBytes);
    }

    // 4. Save updated JSON metadata
    std::string updatedJson;
    serializeJson(doc, updatedJson);
    FileManager::writeFile(jsonPath, updatedJson, 0);

    return result;
}


 // Method to modify an existing element's raw byte data by index
bool fileVector::modify(const std::vector<uint8_t>& data, int i) {
    // i == -1 means the last one;
    // if size is not consistent, error, return false;
    std::string jsonPath = fileName + ".json";
    std::string jsonStr = readTextFile(jsonPath);

    JsonDocument doc;
    if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
        return false;
    }

    JsonArray elements = doc["elements"].as<JsonArray>();
    size_t n = elements.size();
    if (n == 0) {
        return false;
    }

    // Convert negative index (e.g., -1) to positive index
    if (i < 0) {
        i = static_cast<int>(n) + i;
    }

    // Validate index bounds
    if (i < 0 || i >= static_cast<int>(n)) {
        return false;
    }

    JsonObject target = elements[i];
    size_t byteSize = target["byteSize"].as<size_t>();

    // Check if the new data size matches the existing element's byte size
    if (data.size() != byteSize) {
        return false;
    }

    size_t offset = target["offset"].as<size_t>();

    // 1. Write the new byte data into the binary file at the exact offset
    FileManager::writeFile(fileName, data, offset);

    // 2. Optionally update the timestamp to reflect when it was modified
    target["AddingTime"] = static_cast<uint32_t>(esp_timer_get_time() / 1000);

    // 3. Save updated JSON metadata
    std::string updatedJson;
    serializeJson(doc, updatedJson);
    FileManager::writeFile(jsonPath, updatedJson, 0);

    return true;
}


} // namespace file