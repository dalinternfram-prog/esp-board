
#include "file-vector.hpp" // Include the header file containing the fileVector class declaration
#include "esp_timer.h"     // Include ESP-IDF timer functions to get high-resolution timestamps
#include <cstddef>
#include <vector>
namespace file {           // Begin the 'file' namespace for organization

// Constructor implementation for the fileVector class
fileVector::fileVector(std::string fileName, std::shared_ptr<FileManager> FileManager_)
    : FileManager_(std::move(FileManager_)), fileName(std::move(fileName)) { // Initialize member variables, matching the member declaration order
    
    std::string jsonPath = fileName + ".json"; // Construct the metadata file path by appending ".json"
    if (!FileManager_->exists(jsonPath)) {     // Check if the metadata JSON file does not already exist
        JsonDocument doc;                      // Declare an ArduinoJson v7 document to hold metadata
        doc["elements"] = JsonArray();         // Initialize the "elements" key with an empty JSON array
        
        std::string output;                    // Create a string to store the serialized JSON data
        serializeJson(doc, output);            // Serialize the JSON document into the output string
        FileManager_->writeFile(jsonPath, output); // Write the initial JSON string to the metadata file
    }                                          // End of the metadata file creation conditional block
}                                              // End of the constructor definition

// Method to append a new block of raw byte data to the vector
void fileVector::push_back(const std::vector<uint8_t>& data) {
    if (!FileManager_) return;                 // Guard clause: exit early if the file manager pointer is null

    std::string jsonPath = fileName + ".json"; // Build the metadata JSON file path
    std::string jsonStr = FileManager_->readFile(jsonPath); // Read the current metadata JSON content from storage

    JsonDocument doc;                          // Declare a new ArduinoJson v7 document
    DeserializationError error = deserializeJson(doc, jsonStr); // Attempt to parse the JSON string into the document
    
    if (error) {                               // Check if deserialization failed (e.g., corrupted file)
        doc.clear();                           // Clear the corrupted document contents
        doc["elements"] = JsonArray();         // Re-initialize the "elements" key with a fresh empty array
    }                                          // End of the error handling conditional block

    JsonArray elements = doc["elements"].to<JsonArray>(); // Access or convert the "elements" node into a JsonArray

    int currentOffset = 0;                     // Initialize the byte offset for the new element to 0
    if (elements.size() > 0) {                 // Check if there are already existing elements in the array
        JsonObject lastElement = elements[elements.size() - 1]; // Retrieve the last element object in the array
        currentOffset = lastElement["offset"].as<int>() + lastElement["byteSize"].as<size_t>(); // Calculate the next offset based on the last element's position and size
    }                                          // End of the offset calculation conditional block

    FileManager_->appendFile(fileName, data);  // Append the raw byte data to the main data file

    JsonObject newElement = elements.add<JsonObject>(); // Use ArduinoJson v7 syntax to create and add a new nested object
    newElement["byteSize"] = data.size();      // Store the size of the added byte block
    newElement["offset"] = currentOffset;      // Store the calculated byte offset where this element starts
    newElement["AddingTime"] = static_cast<uint32_t>(esp_timer_get_time() / 1000); // Record the current timestamp in milliseconds using the ESP timer

    std::string updatedJson;                   // Create a string to hold the updated JSON metadata
    serializeJson(doc, updatedJson);           // Serialize the modified document into the string
    FileManager_->writeFile(jsonPath, updatedJson); // Write the updated metadata string back to the JSON file
}                                              // End of push_back method definition

// Method to retrieve the current size (number of elements) of the vector
void fileVector::GetCurrentSize() {
    std::string jsonPath = fileName + ".json"; // Construct the metadata JSON file path
    std::string jsonStr = FileManager_->readFile(jsonPath); // Read the JSON metadata file contents
    
    JsonDocument doc;                          // Declare an ArduinoJson v7 document
    if (deserializeJson(doc, jsonStr) == DeserializationError::Ok) { // Check if JSON deserialization succeeds
        JsonArray elements = doc["elements"].as<JsonArray>(); // Extract the "elements" array reference
        // elements.size();                    // Commented-out line representing size retrieval logic
    }                                          // End of the deserialization success conditional block
}                                              // End of GetCurrentSize method definition

// Method to fetch a specific element's raw byte data by its index
std::vector<uint8_t> fileVector::Fetch(int i) {
    std::vector<uint8_t> emptyResult;          // Create an empty vector to return in case of errors
    if (!FileManager_) return emptyResult;     // Guard clause: return empty vector if file manager is null

    std::string jsonPath = fileName + ".json"; // Build the metadata JSON file path
    std::string jsonStr = FileManager_->readFile(jsonPath); // Read the JSON metadata content

    JsonDocument doc;                          // Declare an ArduinoJson v7 document
    if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) { // Check if JSON parsing failed
        return emptyResult;                    // Return the empty result vector on error
    }                                          // End of the parsing error check block

    JsonArray elements = doc["elements"].as<JsonArray>(); // Get the "elements" JSON array
    if (i < 0 || i >= static_cast<int>(elements.size())) { // Validate that the requested index is within bounds
        return emptyResult;                    // Return empty vector if index is out of bounds
    }                                          // End of the bounds checking conditional block

    JsonObject target = elements[i];           // Retrieve the target JSON object at index i
    size_t byteSize = target["byteSize"].as<size_t>(); // Extract the byte size of the target element
    size_t offset = target["offset"].as<size_t>(); // Extract the starting byte offset of the target element (cast to size_t)

    // NEW: Use range-based readFileBytes to read ONLY the target chunk, preventing RAM overflow on large files
    return FileManager_->readFileBytes(fileName, offset, byteSize); // Read and return only the specific byte slice
}                                              // End of Fetch method definition

// Method to remove and return the last element from the vector (Pop operation)
std::vector<uint8_t> fileVector::Pop() {
    std::vector<uint8_t> emptyResult;          // Create an empty vector for error returns
    if (!FileManager_) return emptyResult;     // Guard clause: return empty vector if file manager is null

    std::string jsonPath = fileName + ".json"; // Build the metadata JSON file path
    std::string jsonStr = FileManager_->readFile(jsonPath); // Read the JSON metadata string from storage

    JsonDocument doc;                          // Declare an ArduinoJson v7 document
    if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) { // Verify successful JSON deserialization
        return emptyResult;                    // Return empty vector if deserialization fails
    }                                          // End of deserialization check block

    JsonArray elements = doc["elements"].as<JsonArray>(); // Extract the "elements" JSON array
    if (elements.size() == 0) {                // Check if the vector/array is currently empty
        return emptyResult;                    // Return empty vector if there's nothing to pop
    }                                          // End of empty check conditional block

    JsonObject lastElement = elements[elements.size() - 1]; // Get the last element object from the array
    size_t byteSize = lastElement["byteSize"].as<size_t>(); // Extract the byte size of the last element
    int offset = lastElement["offset"].as<size_t>(); // Extract the starting byte offset of the last element

    std::vector<uint8_t> rawData = FileManager_->readFileBytes(fileName); // Read all raw bytes from the main data file
    std::vector<uint8_t> result;               // Declare a vector to store the popped data result
    
    if (rawData.size() >= static_cast<size_t>(offset + byteSize)) { // Check if the raw data file contains the expected range
        result.assign(rawData.begin() + offset, rawData.begin() + offset + byteSize); // Copy target element bytes into the result vector
        rawData.resize(offset);                // Truncate the raw data vector to remove the popped element
        FileManager_->writeFile(fileName, rawData); // Write the truncated byte stream back to the main data file
    }                                          // End of the data truncation conditional block

    elements.remove(elements.size() - 1);      // Remove the last element entry from the JSON elements array
    std::string updatedJson;                   // Create a string to hold the updated JSON metadata
    serializeJson(doc, updatedJson);           // Serialize the modified JSON document
    FileManager_->writeFile(jsonPath, updatedJson); // Write the updated metadata string back to the file

    return result;                             // Return the popped byte vector
}                                              // End of Pop method definition

 // Method to remove and research the first element using Lazy Compaction FIFO strategy
std::vector<uint8_t> fileVector::PopFront() {
    std::vector<uint8_t> emptyResult;          // Create an empty vector for error returns
    if (!FileManager_) return emptyResult;     // Guard clause: return empty vector if file manager is null

    std::string jsonPath = fileName + ".json"; // Build the metadata JSON file path
    std::string jsonStr = FileManager_->readFile(jsonPath); // Read the JSON metadata file contents

    JsonDocument doc;                          // Declare an ArduinoJson v7 document
    if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) { // Verify successful JSON deserialization
        return emptyResult;                    // Return empty result if parsing fails
    }                                          // End of deserialization check block

    JsonArray elements = doc["elements"].as<JsonArray>(); // Extract the elements JSON array reference
    if (elements.size() == 0) {                // Check if the queue is currently empty
        return emptyResult;                    // Return empty result if there is nothing to pop
    }                                          // End of empty check block

    // 1. Get info about the first element
    JsonObject firstElement = elements[0];     // Retrieve the first element object in the array
    size_t frontByteSize = firstElement["byteSize"].as<size_t>(); // Get the byte size of the front element
    size_t frontOffset = firstElement["offset"].as<size_t>(); // Get the absolute file offset of the front element

    // 2. Read ONLY the popped element using range-based reader (Zero RAM bloat!)
    std::vector<uint8_t> result = FileManager_->readFileBytes(fileName, frontOffset, frontByteSize);

    // 3. Remove the first element reference from the JSON array immediately
    elements.remove(0);                        // Drop the front element index from metadata

    // 4. Lazy Compaction Logic
    const size_t COMPACTION_THRESHOLD = 4096;  // Define waste threshold (4KB) before triggering file compaction
    
    if (elements.size() == 0) {
        // If queue is completely empty, wipe the binary file clean to reclaim all disk space instantly
        FileManager_->writeFile(fileName, std::vector<uint8_t>());
    } 
    else {
        // Check how much dead space has accumulated at the front of the file
        size_t newStartOffset = elements[0]["offset"].as<size_t>();

        if (newStartOffset >= COMPACTION_THRESHOLD) {
            // UPDATED: Delegate file shifting to the FileManager class helper
            FileManager_->shiftBytes(fileName, newStartOffset, true);

            // Shift all remaining JSON element offsets back down to start at 0
            for (JsonObject el : elements) {
                size_t currentElOffset = el["offset"].as<size_t>();
                el["offset"] = currentElOffset - newStartOffset; // Adjust offset relative to the new file start
            }
        }                                      // End of compaction threshold conditional check
    }                                          // End of empty vs non-empty queue check block

    // 5. Save the updated JSON metadata
    std::string updatedJson;                   // Create a string to hold the updated JSON metadata
    serializeJson(doc, updatedJson);           // Serialize the modified document into string format
    FileManager_->writeFile(jsonPath, updatedJson); // Write the updated JSON metadata back to the file

    return result;                             // Return the extracted front byte vector
}                                              // End of PopFront method definition
} // namespace file                            // Close the 'file' namespace







std::vector<uint8_t> fileVeector::Pop(size_t i){
    
}