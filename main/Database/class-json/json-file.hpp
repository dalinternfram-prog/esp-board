



// #include "file_manager.hpp"
// #include <memory>
// #include <string>
// #include <vector>
// #include <type_traits>
// #include <cstring>
// #include <stdexcept>
// #include <typeinfo>

// namespace file { // Begins the json_file namespace wrapper
// //* this class will call vector file,which is another class,the filename will becomes filename and filename.json;

// class JsonFileStore {
// private:
//     std::shared_ptr<FileManager> fileManager_; // Shared file manager instance
//     fileVector fVec_; // Reusable fileVector storage instance

//     // Template function validation for plain structs
//     template<typename T>
//     void check_plain_struct() const {
//         static_assert(std::is_trivially_copyable_v<T>, // Check if type T is trivially copyable at compile-time
//             "Error: T must be a plain struct (trivially copyable)."); // Compilation error message if check fails
//     } // End of check_plain_struct function

//     // Helper function to encapsulate duplicated type-filtering loop
//     template<typename T>
//     std::vector<size_t> get_matching_indices() const {
//         size_t totalElements = fVec_.GetCurrentSize(); // Get total element count from fileVector storage
//         std::vector<size_t> matchingIndices; // Initialize vector to store indices of blocks matching type T
//         if (totalElements == 0) return matchingIndices; // Return empty vector early if the file contains no elements

//         std::string targetTypeName = typeid(T).name(); // Capture the mangled type name string for type T

//         for (size_t j = 0; j < totalElements; ++j) { // Loop through all stored blocks in fileVector
//             std::vector<uint8_t> block = fVec_.Fetch(static_cast<int>(j)); // Fetch raw byte block at current index j
//             if (block.size() < sizeof(size_t)) continue; // Skip block if it is too small to contain type name length

//             size_t offset = 0; // Initialize byte offset tracker for block header parsing
//             size_t nameLen; // Variable to store the length of the type name string
//             std::memcpy(&nameLen, &block[offset], sizeof(size_t)); // Copy type name length bytes from block header
//             offset += sizeof(size_t); // Advance offset tracker past name length field

//             if (block.size() < offset + nameLen + sizeof(size_t)) continue; // Skip block if too small for type name and struct size
//             std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen); // Reconstruct type name string from block bytes
//             offset += nameLen; // Advance offset tracker past the type name string bytes

//             size_t structSize; // Variable to store the size of the stored struct payload
//             std::memcpy(&structSize, &block[offset], sizeof(size_t)); // Copy struct size bytes from block header

//             if (structSize == sizeof(T) && typeName == targetTypeName) { // Check if struct size and type name match type T exactly
//                 matchingIndices.push_back(j); // Record index of matching block into our results vector
//             } // End of match condition check block
//         } // End of for loop iterating through fileVector elements
//         return matchingIndices; // Return the complete list of matching block indices
//     } // End of get_matching_indices helper function

//     // [NEW HELPER]: Resolves logical index 'i' (supporting negative indices) to physical block index
//     template<typename T>
//     size_t resolve_block_index(int i, const std::string& errorContext) const {
//         std::vector<size_t> matchingIndices = get_matching_indices<T>(); // [NEW HELPER]: Fetch all matching indices for type T using first helper
        
//         if (matchingIndices.empty()) { // [NEW HELPER]: Check if any matching instances of type T were found
//             throw std::out_of_range("No instance of type T found " + errorContext + "."); // [NEW HELPER]: Throw exception with context if empty
//         } // [NEW HELPER]: End of empty check conditional block

//         int resolvedIndex = i; // [NEW HELPER]: Initialize resolved index variable with input parameter i
//         if (i < 0) { // [NEW HELPER]: Check if the provided index is negative
//             resolvedIndex = static_cast<int>(matchingIndices.size()) + i; // [NEW HELPER]: Resolve negative index relative to total match count
//         } // [NEW HELPER]: End of negative index evaluation block

//         if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingIndices.size()) { // [NEW HELPER]: Validate bounds of resolved index
//             throw std::out_of_range("Index out of bounds for type T " + errorContext + "."); // [NEW HELPER]: Throw exception if resolved index is out of bounds
//         } // [NEW HELPER]: End of bounds verification conditional block

//         return matchingIndices[resolvedIndex]; // [NEW HELPER]: Return the exact physical block index from the matching list
//     } // [NEW HELPER]: End of resolve_block_index helper function

// public:
//     // [CHANGED]: Updated constructor parameter order to match fileVector(std::string fileName, std::shared_ptr<FileManager> FileManager_)
//     /**
//      * @brief Constructs a JsonFileStore instance, initializing or loading its accompanying JSON metadata file.
//      * @param fileName Base name of the file storage stream.
//      * @param fileManager Shared pointer to the FileManager instance handling low-level disk I/O.
//      */
//     JsonFileStore(const std::string& fileName, std::shared_ptr<FileManager> fileManager)
//         : fileManager_(std::move(fileManager)), fVec_(fileName, fileManager_) {} // [CHANGED]: Initialized fVec_ matching fileVector signature order

//     // [CHANGED]: Added convenience constructor that initializes a default FileManager internally while keeping fileName first
//     /**
//      * @brief Constructs a JsonFileStore instance with a newly created default FileManager.
//      * @param fileName Base name of the file storage stream.
//      */
//     explicit JsonFileStore(const std::string& fileName)
//         : fileManager_(std::make_shared<FileManager>()), fVec_(fileName, fileManager_) {}

//     /**
//      * @brief Saves an instance of struct T into the file using fileVector.
//      */
//     template<typename T>
//     void save(const T& instance) {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         std::string typeName = typeid(T).name(); // Get string representation of type T name
//         size_t nameLen = typeName.size(); // Determine character length of type name string
//         size_t structSize = sizeof(T); // Determine memory size in bytes of struct T

//         std::vector<uint8_t> buffer; // Initialize byte buffer container for binary serialization
//         auto appendBytes = [&](const void* data, size_t size) { // Lambda helper to append raw memory bytes to buffer
//             const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data); // Cast input data pointer to unsigned byte pointer
//             buffer.insert(buffer.end(), bytePtr, bytePtr + size); // Insert block of bytes at the tail of the buffer vector
//         }; // End of lambda definition for appendBytes

//         // Serialize type metadata and payload into a single raw byte block
//         appendBytes(&nameLen, sizeof(nameLen)); // Append type name length metadata to buffer
//         appendBytes(typeName.data(), nameLen); // Append actual type name string characters to buffer
//         appendBytes(&structSize, sizeof(structSize)); // Append struct payload size metadata to buffer
//         appendBytes(&instance, structSize); // Append raw struct instance memory bytes to buffer

//         // [CHANGED]: Delegate raw block storage and metadata indexing to the reusable member fileVector instance
//         fVec_.push_back(buffer); // Push serialized byte buffer into reusable fileVector storage
//     } // End of save function

//     /**
//      * @brief Modifies/updates the i-th instance of type T in-place.
//      * @param instance The new struct instance containing updated data.
//      * @param i Index of the instance of type T to modify (supports negative indexing, default -1 for the last instance).
//      */
//     template<typename T>
//     void modify(const T& instance, int i = -1) {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         // [CHANGED]: Uses the member fileVector instance directly
//         size_t blockIndexToModify = resolve_block_index<T>(i, "to modify"); // Resolved physical block index using helper function

//         // 3. Fetch the existing block to preserve its header metadata (type name, size, etc.)
//         std::vector<uint8_t> block = fVec_.Fetch(static_cast<int>(blockIndexToModify)); // Fetch existing target block bytes from fileVector

//         size_t offset = 0; // Initialize parsing offset tracker variable
//         size_t nameLen; // Variable to store extracted name length
//         std::memcpy(&nameLen, &block[offset], sizeof(size_t)); // Copy name length from header bytes
//         offset += sizeof(size_t) + nameLen; // Advance offset past name length field and name string characters

//         size_t structSize; // Variable to store extracted struct size
//         std::memcpy(&structSize, &block[offset], sizeof(size_t)); // Copy struct size from header bytes
//         offset += sizeof(size_t); // Advance offset past struct size metadata field

//         if (structSize != sizeof(T)) { // Verify that stored struct size matches current type T size
//             throw std::runtime_error("Struct size mismatch during modification."); // Throw error if sizes do not match
//         } // End of size verification conditional block

//         // 4. Overwrite the old struct payload bytes with the new instance bytes inside the block buffer
//         std::memcpy(&block[offset], &instance, sizeof(T)); // Copy new instance memory representation over old payload bytes

//         // 5 & 6. Delegate the binary write, size checking, and JSON metadata timestamp update to fileVector::modify
//         bool success = fVec_.modify(block, static_cast<int>(blockIndexToModify)); // Call fileVector modify method with updated block
//         if (!success) { // Check if the file write operation failed
//             throw std::runtime_error("Failed to write modified block to fileVector storage."); // Throw error if write failed
//         } // End of success check conditional block
//     } // End of modify function

//     /**
//      * @brief Returns the total count of records matching type T in the file vector.
//      */
//     template<typename T>
//     size_t GetSize() const {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         return get_matching_indices<T>().size(); // Replaced manual loop with helper function call and returned collection size
//     } // End of GetSize function

//     /**
//      * @brief Fetches the i-th instance of type T using fileVector.
//      */
//     template<typename T>
//     T fetch(int i) const {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         size_t blockIndex = resolve_block_index<T>(i, "to fetch"); // Resolved physical block index using helper function

//         std::vector<uint8_t> block = fVec_.Fetch(static_cast<int>(blockIndex)); // Fetch the specific matching byte block from fileVector

//         size_t offset = 0; // Initialize parsing offset tracker variable
//         size_t nameLen; // Variable to store extracted type name length
//         std::memcpy(&nameLen, &block[offset], sizeof(size_t)); // Copy name length from header bytes
//         offset += sizeof(size_t) + nameLen; // Advance offset past name length field and name string characters

//         size_t structSize; // Variable to store extracted struct size
//         std::memcpy(&structSize, &block[offset], sizeof(size_t)); // Copy struct size from header bytes
//         offset += sizeof(size_t); // Advance offset past struct size metadata field

//         T instance; // Declare local instance object of struct type T
//         std::memcpy(&instance, &block[offset], sizeof(T)); // Copy raw payload bytes from block buffer into struct instance
//         return instance; // Return the successfully deserialized struct instance
//     } // End of fetch function

//     /**
//      * @brief Removes and returns the i-th instance of type T (default i = -1 pops the last instance) using fileVector::Pop.
//      */
//     template<typename T>
//     T Pop(int i = -1) {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         size_t blockIndexToPop = resolve_block_index<T>(i, "to pop"); // Resolved physical block index using helper function

//         // Since fileVector's Pop() removes by exact vector index, if our target isn't at the end, 
//         // we can fetch it, then use fileVector's Pop API or handle index removal. 
//         // For efficiency, we leverage fileVector's generalized Pop method if available, 
//         // or call fVec.Pop(blockIndexToPop).
//         std::vector<uint8_t> poppedBlock = fVec_.Pop(static_cast<int>(blockIndexToPop)); // Pop block from fileVector storage using block index

//         // Extract the struct instance from the popped block bytes
//         size_t offset = 0; // Initialize parsing offset tracker variable
//         size_t nameLen; // Variable to store extracted type name length
//         std::memcpy(&nameLen, &poppedBlock[offset], sizeof(size_t)); // Copy name length from header bytes
//         offset += sizeof(size_t) + nameLen; // Advance offset past name length field and name string characters

//         size_t structSize; // Variable to store extracted struct size
//         std::memcpy(&structSize, &poppedBlock[offset], sizeof(size_t)); // Copy struct size from header bytes
//         offset += sizeof(size_t); // Advance offset past struct size metadata field

//         T instance; // Declare local instance object of struct type T
//         std::memcpy(&instance, &poppedBlock[offset], sizeof(T)); // Copy raw payload bytes from popped block into struct instance

//         return instance; // Return the popped struct instance
//     } // End of Pop function

// } // Closes the JsonFileStore class

// } // Closes the json_file namespace wrapper
















#pragma once
#include "esp_log.h"
#include "file_manager.hpp"
#include "file-vector.hpp"
#include <memory>
#include <string>
#include <vector>
#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <typeinfo>

namespace file { // Begins the json_file namespace wrapper
//* this class will call vector file,which is another class,the filename will becomes filename and filename.json;

class JsonFileStore {
private:
    fileVector fVec_; // Reusable fileVector storage instance

    // Template function validation for plain structs
    template<typename T>
    void check_plain_struct() const {
        static_assert(std::is_trivially_copyable_v<T>, // Check if type T is trivially copyable at compile-time
            "Error: T must be a plain struct (trivially copyable)."); // Compilation error message if check fails
    } // End of check_plain_struct function

    // Helper function to encapsulate duplicated type-filtering loop
    template<typename T>
    std::vector<size_t> get_matching_indices() const {
        size_t totalElements = fVec_.GetCurrentSize(); // Get total element count from fileVector storage
        std::vector<size_t> matchingIndices; // Initialize vector to store indices of blocks matching type T
        if (totalElements == 0) return matchingIndices; // Return empty vector early if the file contains no elements

        std::string targetTypeName = typeid(T).name(); // Capture the mangled type name string for type T

        for (size_t j = 0; j < totalElements; ++j) { // Loop through all stored blocks in fileVector
            std::vector<uint8_t> block = fVec_.Fetch(static_cast<int>(j)); // Fetch raw byte block at current index j
            if (block.size() < sizeof(size_t)) continue; // Skip block if it is too small to contain type name length

            size_t offset = 0; // Initialize byte offset tracker for block header parsing
            size_t nameLen; // Variable to store the length of the type name string
            std::memcpy(&nameLen, &block[offset], sizeof(size_t)); // Copy type name length bytes from block header
            offset += sizeof(size_t); // Advance offset tracker past name length field

            if (block.size() < offset + nameLen + sizeof(size_t)) continue; // Skip block if too small for type name and struct size
            std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen); // Reconstruct type name string from block bytes
            offset += nameLen; // Advance offset tracker past the type name string bytes

            size_t structSize; // Variable to store the size of the stored struct payload
            std::memcpy(&structSize, &block[offset], sizeof(size_t)); // Copy struct size bytes from block header

            if (structSize == sizeof(T) && typeName == targetTypeName) { // Check if struct size and type name match type T exactly
                matchingIndices.push_back(j); // Record index of matching block into our results vector
            } // End of match condition check block
        } // End of for loop iterating through fileVector elements
        return matchingIndices; // Return the complete list of matching block indices
    } // End of get_matching_indices helper function

    // [NEW HELPER]: Resolves logical index 'i' (supporting negative indices) to physical block index
    template<typename T>
    size_t resolve_block_index(int i, const std::string& errorContext) const {
        std::vector<size_t> matchingIndices = get_matching_indices<T>(); // [NEW HELPER]: Fetch all matching indices for type T using first helper
        
        if (matchingIndices.empty()) { // [NEW HELPER]: Check if any matching instances of type T were found
            ESP_LOGE("JsonFileStore", "No instance of type T found %s.", errorContext.c_str());
            return {}; // or handle error gracefully
         //   throw std::out_of_range("No instance of type T found " + errorContext + "."); // [NEW HELPER]: Throw exception with context if empty
        } // [NEW HELPER]: End of empty check conditional block

        int resolvedIndex = i; // [NEW HELPER]: Initialize resolved index variable with input parameter i
        if (i < 0) { // [NEW HELPER]: Check if the provided index is negative
            resolvedIndex = static_cast<int>(matchingIndices.size()) + i; // [NEW HELPER]: Resolve negative index relative to total match count
        } // [NEW HELPER]: End of negative index evaluation block

        if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingIndices.size()) { // [NEW HELPER]: Validate bounds of resolved index
            ESP_LOGE("JsonFileStore", "Index out of bounds for type T %s.", errorContext.c_str());
            return {}; // Or handle the failure appropriately in your caller
           // throw std::out_of_range("Index out of bounds for type T " + errorContext + "."); // [NEW HELPER]: Throw exception if resolved index is out of bounds
        } // [NEW HELPER]: End of bounds verification conditional block

        return matchingIndices[resolvedIndex]; // [NEW HELPER]: Return the exact physical block index from the matching list
    } // [NEW HELPER]: End of resolve_block_index helper function

public:
    /**
     * @brief Constructs a JsonFileStore instance, initializing or loading its accompanying JSON metadata file.
     * @param fileName Base name of the file storage stream.
     */
    explicit JsonFileStore(const std::string& fileName)
        : fVec_(fileName) {} // [CHANGED]: Initialized fVec_ directly using the new fileVector signature

    /**
     * @brief Saves an instance of struct T into the file using fileVector.
     */
    template<typename T>
    void save(const T& instance) {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        std::string typeName = typeid(T).name(); // Get string representation of type T name
        size_t nameLen = typeName.size(); // Determine character length of type name string
        size_t structSize = sizeof(T); // Determine memory size in bytes of struct T

        std::vector<uint8_t> buffer; // Initialize byte buffer container for binary serialization
        auto appendBytes = [&](const void* data, size_t size) { // Lambda helper to append raw memory bytes to buffer
            const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data); // Cast input data pointer to unsigned byte pointer
            buffer.insert(buffer.end(), bytePtr, bytePtr + size); // Insert block of bytes at the tail of the buffer vector
        }; // End of lambda definition for appendBytes

        // Serialize type metadata and payload into a single raw byte block
        appendBytes(&nameLen, sizeof(nameLen)); // Append type name length metadata to buffer
        appendBytes(typeName.data(), nameLen); // Append actual type name string characters to buffer
        appendBytes(&structSize, sizeof(structSize)); // Append struct payload size metadata to buffer
        appendBytes(&instance, structSize); // Append raw struct instance memory bytes to buffer

        // [CHANGED]: Delegate raw block storage and metadata indexing to the reusable member fileVector instance
        fVec_.push_back(buffer); // Push serialized byte buffer into reusable fileVector storage
    } // End of save function

    /**
     * @brief Modifies/updates the i-th instance of type T in-place.
     * @param instance The new struct instance containing updated data.
     * @param i Index of the instance of type T to modify (supports negative indexing, default -1 for the last instance).
     */
    template<typename T>
    void modify(const T& instance, int i = -1) {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        // [CHANGED]: Uses the member fileVector instance directly
        size_t blockIndexToModify = resolve_block_index<T>(i, "to modify"); // Resolved physical block index using helper function

        // 3. Fetch the existing block to preserve its header metadata (type name, size, etc.)
        std::vector<uint8_t> block = fVec_.Fetch(static_cast<int>(blockIndexToModify)); // Fetch existing target block bytes from fileVector

        size_t offset = 0; // Initialize parsing offset tracker variable
        size_t nameLen; // Variable to store extracted name length
        std::memcpy(&nameLen, &block[offset], sizeof(size_t)); // Copy name length from header bytes
        offset += sizeof(size_t) + nameLen; // Advance offset past name length field and name string characters

        size_t structSize; // Variable to store extracted struct size
        std::memcpy(&structSize, &block[offset], sizeof(size_t)); // Copy struct size from header bytes
        offset += sizeof(size_t); // Advance offset past struct size metadata field

        if (structSize != sizeof(T)) { // Verify that stored struct size matches current type T size
            throw std::runtime_error("Struct size mismatch during modification."); // Throw error if sizes do not match
        } // End of size verification conditional block

        // 4. Overwrite the old struct payload bytes with the new instance bytes inside the block buffer
        std::memcpy(&block[offset], &instance, sizeof(T)); // Copy new instance memory representation over old payload bytes

        // 5 & 6. Delegate the binary write, size checking, and JSON metadata timestamp update to fileVector::modify
        bool success = fVec_.modify(block, static_cast<int>(blockIndexToModify)); // Call fileVector modify method with updated block
        if (!success) { // Check if the file write operation failed
            throw std::runtime_error("Failed to write modified block to fileVector storage."); // Throw error if write failed
        } // End of success check conditional block
    } // End of modify function

    /**
     * @brief Returns the total count of records matching type T in the file vector.
     */
    template<typename T>
    size_t GetSize() const {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        return get_matching_indices<T>().size(); // Replaced manual loop with helper function call and returned collection size
    } // End of GetSize function

    /**
     * @brief Fetches the i-th instance of type T using fileVector.
     */
    template<typename T>
    T fetch(int i) const {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        size_t blockIndex = resolve_block_index<T>(i, "to fetch"); // Resolved physical block index using helper function

        std::vector<uint8_t> block = fVec_.Fetch(static_cast<int>(blockIndex)); // Fetch the specific matching byte block from fileVector

        size_t offset = 0; // Initialize parsing offset tracker variable
        size_t nameLen; // Variable to store extracted type name length
        std::memcpy(&nameLen, &block[offset], sizeof(size_t)); // Copy name length from header bytes
        offset += sizeof(size_t) + nameLen; // Advance offset past name length field and name string characters

        size_t structSize; // Variable to store extracted struct size
        std::memcpy(&structSize, &block[offset], sizeof(size_t)); // Copy struct size from header bytes
        offset += sizeof(size_t); // Advance offset past struct size metadata field

        T instance; // Declare local instance object of struct type T
        std::memcpy(&instance, &block[offset], sizeof(T)); // Copy raw payload bytes from block buffer into struct instance
        return instance; // Return the successfully deserialized struct instance
    } // End of fetch function

    /**
     * @brief Removes and returns the i-th instance of type T (default i = -1 pops the last instance) using fileVector::Pop.
     */
    template<typename T>
    T Pop(int i = -1) {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        size_t blockIndexToPop = resolve_block_index<T>(i, "to pop"); // Resolved physical block index using helper function

        // Since fileVector's Pop() removes by exact vector index, if our target isn't at the end, 
        // we can fetch it, then use fileVector's Pop API or handle index removal. 
        // For efficiency, we leverage fileVector's generalized Pop method if available, 
        // or call fVec.Pop(blockIndexToPop).
        std::vector<uint8_t> poppedBlock = fVec_.Pop(static_cast<int>(blockIndexToPop)); // Pop block from fileVector storage using block index

        // Extract the struct instance from the popped block bytes
        size_t offset = 0; // Initialize parsing offset tracker variable
        size_t nameLen; // Variable to store extracted type name length
        std::memcpy(&nameLen, &poppedBlock[offset], sizeof(size_t)); // Copy name length from header bytes
        offset += sizeof(size_t) + nameLen; // Advance offset past name length field and name string characters

        size_t structSize; // Variable to store extracted struct size
        std::memcpy(&structSize, &poppedBlock[offset], sizeof(size_t)); // Copy struct size from header bytes
        offset += sizeof(size_t); // Advance offset past struct size metadata field

        T instance; // Declare local instance object of struct type T
        std::memcpy(&instance, &poppedBlock[offset], sizeof(T)); // Copy raw payload bytes from popped block into struct instance

        return instance; // Return the popped struct instance
    } // End of Pop function

}; // Closes the JsonFileStore class

} // Closes the json_file namespace wrapper