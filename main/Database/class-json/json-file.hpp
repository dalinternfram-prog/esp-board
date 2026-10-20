




// // namespace json_file { // Begins the json_file namespace wrapper

// //     inline std::shared_ptr<FileManager> fileManager = std::make_shared<FileManager>(); // Shared file manager instance

// //     template<typename T> // Template function validation for plain structs
// //     void check_plain_struct() {
// //         static_assert(std::is_trivially_copyable_v<T>, 
// //             "Error: T must be a plain struct (trivially copyable).");
// //     }

// //     /**
// //      * @brief Saves an instance of struct T into the file using fileVector.
// //      */
// //     template<typename T>
// //     void save(const T& instance, const std::string& fileName) {
// //         check_plain_struct<T>();

// //         std::string typeName = typeid(T).name();
// //         size_t nameLen = typeName.size();
// //         size_t structSize = sizeof(T);

// //         std::vector<uint8_t> buffer;
// //         auto appendBytes = [&](const void* data, size_t size) {
// //             const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data);
// //             buffer.insert(buffer.end(), bytePtr, bytePtr + size);
// //         };

// //         // Serialize type metadata and payload into a single raw byte block
// //         appendBytes(&nameLen, sizeof(nameLen));
// //         appendBytes(typeName.data(), nameLen);
// //         appendBytes(&structSize, sizeof(structSize));
// //         appendBytes(&instance, structSize);

// //         // Delegate raw block storage and metadata indexing to fileVector
// //         fileVector fVec(fileName, fileManager);
// //         fVec.push_back(buffer);
// //     }


// //  /**
// //      * @brief Modifies/updates the i-th instance of type T in-place.
// //      * @param instance The new struct instance containing updated data.
// //      * @param fileName The base name of the file storage.
// //      * @param i Index of the instance of type T to modify (supports negative indexing, default -1 for the last instance).
// //      */
// //     template<typename T>
// //     void modify(const T& instance, const std::string& fileName, int i = -1) {
// //         check_plain_struct<T>();

// //         fileVector fVec(fileName, fileManager);
// //         size_t totalElements = fVec.GetCurrentSize();
// //         if (totalElements == 0) {
// //             throw std::runtime_error("File is empty or does not exist.");
// //         }

// //         std::string targetTypeName = typeid(T).name();
// //         std::vector<size_t> matchingBlockIndices;

// //         // 1. Find all fileVector block indices that match type T
// //         for (size_t j = 0; j < totalElements; ++j) {
// //             std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(j));
// //             if (block.size() < sizeof(size_t)) continue;

// //             size_t offset = 0;
// //             size_t nameLen;
// //             std::memcpy(&nameLen, &block[offset], sizeof(size_t));
// //             offset += sizeof(size_t);

// //             if (block.size() < offset + nameLen + sizeof(size_t)) continue;
// //             std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen);
// //             offset += nameLen;

// //             size_t structSize;
// //             std::memcpy(&structSize, &block[offset], sizeof(size_t));

// //             if (structSize == sizeof(T) && typeName == targetTypeName) {
// //                 matchingBlockIndices.push_back(j);
// //             }
// //         }

// //         if (matchingBlockIndices.empty()) {
// //             throw std::out_of_range("No instance of type T found to modify.");
// //         }

// //         // 2. Resolve negative index (e.g., -1 for the last instance)
// //         int resolvedIndex = i;
// //         if (i < 0) {
// //             resolvedIndex = static_cast<int>(matchingBlockIndices.size()) + i;
// //         }

// //         if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingBlockIndices.size()) {
// //             throw std::out_of_range("Index out of bounds for type T modification.");
// //         }

// //         size_t blockIndexToModify = matchingBlockIndices[resolvedIndex];

// //         // 3. Fetch the existing block to preserve its header metadata (type name, size, etc.)
// //         std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(blockIndexToModify));

// //         size_t offset = 0;
// //         size_t nameLen;
// //         std::memcpy(&nameLen, &block[offset], sizeof(size_t));
// //         offset += sizeof(size_t) + nameLen;

// //         size_t structSize;
// //         std::memcpy(&structSize, &block[offset], sizeof(size_t));
// //         offset += sizeof(size_t);

// //         if (structSize != sizeof(T)) {
// //             throw std::runtime_error("Struct size mismatch during modification.");
// //         }

// //         // 4. Overwrite the old struct payload bytes with the new instance bytes inside the block buffer
// //         std::memcpy(&block[offset], &instance, sizeof(T));

// //         // 5 & 6. Delegate the binary write, size checking, and JSON metadata timestamp update to fileVector::modify
// //         bool success = fVec.modify(block, static_cast<int>(blockIndexToModify));
// //         if (!success) {
// //             throw std::runtime_error("Failed to write modified block to fileVector storage.");
// //         }
// //     }




    
// //     /**
// //      * @brief Returns the total count of records matching type T in the file vector.
// //      */
// //     template<typename T>
// //     size_t GetSize(const std::string& fileName) {
// //         check_plain_struct<T>();

// //         fileVector fVec(fileName, fileManager);
// //         size_t totalElements = fVec.GetCurrentSize();
// //         if (totalElements == 0) return 0;

// //         size_t matchingCount = 0;
// //         std::string targetTypeName = typeid(T).name();

// //         // Iterate through stored blocks using fileVector::Fetch
// //         for (size_t i = 0; i < totalElements; ++i) {
// //             std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(i));
// //             if (block.size() < sizeof(size_t)) continue;

// //             size_t offset = 0;
// //             size_t nameLen;
// //             std::memcpy(&nameLen, &block[offset], sizeof(size_t));
// //             offset += sizeof(size_t);

// //             if (block.size() < offset + nameLen + sizeof(size_t)) continue;
// //             std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen);
// //             offset += nameLen;

// //             size_t structSize;
// //             std::memcpy(&structSize, &block[offset], sizeof(size_t));

// //             if (structSize == sizeof(T) && typeName == targetTypeName) {
// //                 matchingCount++;
// //             }
// //         }

// //         return matchingCount;
// //     }

// //     /**
// //      * @brief Fetches the i-th instance of type T using fileVector.
// //      */
// //     template<typename T>
// //     T fetch(int i, const std::string& fileName) {
// //         check_plain_struct<T>();

// //         fileVector fVec(fileName, fileManager);
// //         size_t totalElements = fVec.GetCurrentSize();
// //         if (totalElements == 0) {
// //             throw std::runtime_error("File is empty or does not exist.");
// //         }

// //         std::string targetTypeName = typeid(T).name();
// //         int targetIndexCount = 0;

// //         for (size_t j = 0; j < totalElements; ++j) {
// //             std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(j));
// //             if (block.size() < sizeof(size_t)) continue;

// //             size_t offset = 0;
// //             size_t nameLen;
// //             std::memcpy(&nameLen, &block[offset], sizeof(size_t));
// //             offset += sizeof(size_t);

// //             if (block.size() < offset + nameLen + sizeof(size_t)) continue;
// //             std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen);
// //             offset += nameLen;

// //             size_t structSize;
// //             std::memcpy(&structSize, &block[offset], sizeof(size_t));
// //             offset += sizeof(size_t);

// //             if (structSize == sizeof(T) && typeName == targetTypeName) {
// //                 if (targetIndexCount == i) {
// //                     T instance;
// //                     std::memcpy(&instance, &block[offset], sizeof(T));
// //                     return instance;
// //                 }
// //                 targetIndexCount++;
// //             }
// //         }

// //         throw std::out_of_range("Index out of bounds for type T.");
// //     }

// //     /**
// //      * @brief Removes and returns the i-th instance of type T (default i = -1 pops the last instance) using fileVector::Pop.
// //      */
// //     template<typename T>
// //     T Pop(const std::string& fileName, int i = -1) {
// //         check_plain_struct<T>();

// //         fileVector fVec(fileName, fileManager);
// //         size_t totalElements = fVec.GetCurrentSize();
// //         if (totalElements == 0) {
// //             throw std::runtime_error("File is empty or does not exist.");
// //         }

// //         std::string targetTypeName = typeid(T).name();
// //         std::vector<size_t> matchingBlockIndices;

// //         // Find all fileVector block indices that match type T
// //         for (size_t j = 0; j < totalElements; ++j) {
// //             std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(j));
// //             if (block.size() < sizeof(size_t)) continue;

// //             size_t offset = 0;
// //             size_t nameLen;
// //             std::memcpy(&nameLen, &block[offset], sizeof(size_t));
// //             offset += sizeof(size_t);

// //             if (block.size() < offset + nameLen + sizeof(size_t)) continue;
// //             std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen);
// //             offset += nameLen;

// //             size_t structSize;
// //             std::memcpy(&structSize, &block[offset], sizeof(size_t));

// //             if (structSize == sizeof(T) && typeName == targetTypeName) {
// //                 matchingBlockIndices.push_back(j);
// //             }
// //         }

// //         if (matchingBlockIndices.empty()) {
// //             throw std::out_of_range("No instance of type T found to pop.");
// //         }

// //         // Handle negative indexing (e.g., -1 for the last element)
// //         int resolvedIndex = i;
// //         if (i < 0) {
// //             resolvedIndex = static_cast<int>(matchingBlockIndices.size()) + i;
// //         }

// //         if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingBlockIndices.size()) {
// //             throw std::out_of_range("Index out of bounds for type T pop.");
// //         }

// //         size_t blockIndexToPop = matchingBlockIndices[resolvedIndex];

// //         // Since fileVector's Pop() removes by exact vector index, if our target isn't at the end, 
// //         // we can fetch it, then use fileVector's Pop API or handle index removal. 
// //         // For efficiency, we leverage fileVector's generalized Pop method if available, 
// //         // or call fVec.Pop(blockIndexToPop).
// //         std::vector<uint8_t> poppedBlock = fVec.Pop(static_cast<int>(blockIndexToPop));

// //         // Extract the struct instance from the popped block bytes
// //         size_t offset = 0;
// //         size_t nameLen;
// //         std::memcpy(&nameLen, &poppedBlock[offset], sizeof(size_t));
// //         offset += sizeof(size_t) + nameLen;

// //         size_t structSize;
// //         std::memcpy(&structSize, &poppedBlock[offset], sizeof(size_t));
// //         offset += sizeof(size_t);

// //         T instance;
// //         std::memcpy(&instance, &poppedBlock[offset], sizeof(T));

// //         return instance;
// //     }

// // } // Closes the json_file namespace

























// namespace json_file { // Begins the json_file namespace wrapper

//     inline std::shared_ptr<FileManager> fileManager = std::make_shared<FileManager>(); // Shared file manager instance

//     template<typename T> // Template function validation for plain structs
//     void check_plain_struct() {
//         static_assert(std::is_trivially_copyable_v<T>, // Check if type T is trivially copyable at compile-time
//             "Error: T must be a plain struct (trivially copyable)."); // Compilation error message if check fails
//     } // End of check_plain_struct function

//     namespace { // [ADDED]: Begins an anonymous namespace for internal helper linkage
//         template<typename T> // [ADDED]: Template function declaration for finding matching block indices
//         std::vector<size_t> get_matching_indices(fileVector& fVec) { // [ADDED/REFACTORED]: Helper function to encapsulate duplicated type-filtering loop
//             size_t totalElements = fVec.GetCurrentSize(); // [ADDED]: Get total element count from fileVector storage
//             std::vector<size_t> matchingIndices; // [ADDED]: Initialize vector to store indices of blocks matching type T
//             if (totalElements == 0) return matchingIndices; // [ADDED]: Return empty vector early if the file contains no elements

//             std::string targetTypeName = typeid(T).name(); // [ADDED]: Capture the mangled type name string for type T

//             for (size_t j = 0; j < totalElements; ++j) { // [ADDED]: Loop through all stored blocks in fileVector
//                 std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(j)); // [ADDED]: Fetch raw byte block at current index j
//                 if (block.size() < sizeof(size_t)) continue; // [ADDED]: Skip block if it is too small to contain type name length

//                 size_t offset = 0; // [ADDED]: Initialize byte offset tracker for block header parsing
//                 size_t nameLen; // [ADDED]: Variable to store the length of the type name string
//                 std::memcpy(&nameLen, &block[offset], sizeof(size_t)); // [ADDED]: Copy type name length bytes from block header
//                 offset += sizeof(size_t); // [ADDED]: Advance offset tracker past name length field

//                 if (block.size() < offset + nameLen + sizeof(size_t)) continue; // [ADDED]: Skip block if too small for type name and struct size
//                 std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen); // [ADDED]: Reconstruct type name string from block bytes
//                 offset += nameLen; // [ADDED]: Advance offset tracker past the type name string bytes

//                 size_t structSize; // [ADDED]: Variable to store the size of the stored struct payload
//                 std::memcpy(&structSize, &block[offset], sizeof(size_t)); // [ADDED]: Copy struct size bytes from block header

//                 if (structSize == sizeof(T) && typeName == targetTypeName) { // [ADDED]: Check if struct size and type name match type T exactly
//                     matchingIndices.push_back(j); // [ADDED]: Record index of matching block into our results vector
//                 } // [ADDED]: End of match condition check block
//             } // [ADDED]: End of for loop iterating through fileVector elements
//             return matchingIndices; // [ADDED]: Return the complete list of matching block indices
//         } // [ADDED]: End of get_matching_indices helper function
//     } // [ADDED]: Closes the anonymous namespace

//     /**
//      * @brief Saves an instance of struct T into the file using fileVector.
//      */
//     template<typename T>
//     void save(const T& instance, const std::string& fileName) {
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

//         // Delegate raw block storage and metadata indexing to fileVector
//         fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
//         fVec.push_back(buffer); // Push serialized byte buffer into fileVector storage
//     } // End of save function

//     /**
//      * @brief Modifies/updates the i-th instance of type T in-place.
//      * @param instance The new struct instance containing updated data.
//      * @param fileName The base name of the file storage.
//      * @param i Index of the instance of type T to modify (supports negative indexing, default -1 for the last instance).
//      */
//     template<typename T>
//     void modify(const T& instance, const std::string& fileName, int i = -1) {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
//         std::vector<size_t> matchingBlockIndices = get_matching_indices<T>(fVec); // [REFACTORED]: Replaced manual loop with clean call to helper function

//         if (matchingBlockIndices.empty()) { // Check if any matching instances of type T were found
//             throw std::out_of_range("No instance of type T found to modify."); // Throw out of range exception if no matches exist
//         } // End of empty check conditional block

//         // 2. Resolve negative index (e.g., -1 for the last instance)
//         int resolvedIndex = i; // Initialize resolved index variable with input parameter i
//         if (i < 0) { // Check if the provided index is negative
//             resolvedIndex = static_cast<int>(matchingBlockIndices.size()) + i; // Resolve negative index relative to total match count
//         } // End of negative index evaluation block

//         if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingBlockIndices.size()) { // Validate bounds of resolved index
//             throw std::out_of_range("Index out of bounds for type T modification."); // Throw exception if resolved index is out of bounds
//         } // End of bounds verification conditional block

//         size_t blockIndexToModify = matchingBlockIndices[resolvedIndex]; // Extract actual physical fileVector block index from matched indices

//         // 3. Fetch the existing block to preserve its header metadata (type name, size, etc.)
//         std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(blockIndexToModify)); // Fetch existing target block bytes from fileVector

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
//         bool success = fVec.modify(block, static_cast<int>(blockIndexToModify)); // Call fileVector modify method with updated block
//         if (!success) { // Check if the file write operation failed
//             throw std::runtime_error("Failed to write modified block to fileVector storage."); // Throw error if write failed
//         } // End of success check conditional block
//     } // End of modify function

//     /**
//      * @brief Returns the total count of records matching type T in the file vector.
//      */
//     template<typename T>
//     size_t GetSize(const std::string& fileName) {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
//         return get_matching_indices<T>(fVec).size(); // [REFACTORED]: Replaced manual loop with helper function call and returned collection size
//     } // End of GetSize function

//     /**
//      * @brief Fetches the i-th instance of type T using fileVector.
//      */
//     template<typename T>
//     T fetch(int i, const std::string& fileName) {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
//         std::vector<size_t> matchingBlockIndices = get_matching_indices<T>(fVec); // [REFACTORED]: Replaced manual loop with helper function call

//         if (i < 0 || static_cast<size_t>(i) >= matchingBlockIndices.size()) { // [REFACTORED]: Validate index directly against matching block indices list
//             throw std::out_of_range("Index out of bounds for type T."); // Throw out of range exception if index is invalid
//         } // End of bounds verification conditional block

//         size_t blockIndex = matchingBlockIndices[i]; // [REFACTORED]: Retrieve exact physical block index corresponding to match index i
//         std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(blockIndex)); // [REFACTORED]: Fetch the specific matching byte block from fileVector

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
//     T Pop(const std::string& fileName, int i = -1) {
//         check_plain_struct<T>(); // Validate that template parameter T is a plain struct

//         fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
//         std::vector<size_t> matchingBlockIndices = get_matching_indices<T>(fVec); // [REFACTORED]: Replaced manual loop with helper function call

//         if (matchingBlockIndices.empty()) { // Check if any matching blocks exist for type T
//             throw std::out_of_range("No instance of type T found to pop."); // Throw out of range exception if none found
//         } // End of empty check conditional block

//         // Handle negative indexing (e.g., -1 for the last element)
//         int resolvedIndex = i; // Initialize resolved index variable with input parameter i
//         if (i < 0) { // Check if input index parameter is negative
//             resolvedIndex = static_cast<int>(matchingBlockIndices.size()) + i; // Resolve negative index relative to total match count
//         } // End of negative index evaluation block

//         if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingBlockIndices.size()) { // Validate bounds of resolved index
//             throw std::out_of_range("Index out of bounds for type T pop."); // Throw exception if resolved index is invalid
//         } // End of bounds verification conditional block

//         size_t blockIndexToPop = matchingBlockIndices[resolvedIndex]; // Extract physical block index to pop from matching indices list

//         // Since fileVector's Pop() removes by exact vector index, if our target isn't at the end, 
//         // we can fetch it, then use fileVector's Pop API or handle index removal. 
//         // For efficiency, we leverage fileVector's generalized Pop method if available, 
//         // or call fVec.Pop(blockIndexToPop).
//         std::vector<uint8_t> poppedBlock = fVec.Pop(static_cast<int>(blockIndexToPop)); // Pop block from fileVector storage using block index

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

// } // Closes the json_file namespace wrapper















namespace json_file { // Begins the json_file namespace wrapper

    inline std::shared_ptr<FileManager> fileManager = std::make_shared<FileManager>(); // Shared file manager instance

    template<typename T> // Template function validation for plain structs
    void check_plain_struct() {
        static_assert(std::is_trivially_copyable_v<T>, // Check if type T is trivially copyable at compile-time
            "Error: T must be a plain struct (trivially copyable)."); // Compilation error message if check fails
    } // End of check_plain_struct function

    namespace { // Begins an anonymous namespace for internal helper linkage
        
        template<typename T> // Template function declaration for finding matching block indices
        std::vector<size_t> get_matching_indices(fileVector& fVec) { // Helper function to encapsulate duplicated type-filtering loop
            size_t totalElements = fVec.GetCurrentSize(); // Get total element count from fileVector storage
            std::vector<size_t> matchingIndices; // Initialize vector to store indices of blocks matching type T
            if (totalElements == 0) return matchingIndices; // Return empty vector early if the file contains no elements

            std::string targetTypeName = typeid(T).name(); // Capture the mangled type name string for type T

            for (size_t j = 0; j < totalElements; ++j) { // Loop through all stored blocks in fileVector
                std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(j)); // Fetch raw byte block at current index j
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

        template<typename T> // [NEW HELPER]: Template function declaration for resolving logical index to physical block index
        size_t resolve_block_index(fileVector& fVec, int i, const std::string& errorContext) { // [NEW HELPER]: Resolves logical index 'i' (supporting negative indices) to physical block index
            std::vector<size_t> matchingIndices = get_matching_indices<T>(fVec); // [NEW HELPER]: Fetch all matching indices for type T using first helper
            
            if (matchingIndices.empty()) { // [NEW HELPER]: Check if any matching instances of type T were found
                throw std::out_of_range("No instance of type T found " + errorContext + "."); // [NEW HELPER]: Throw exception with context if empty
            } // [NEW HELPER]: End of empty check conditional block

            int resolvedIndex = i; // [NEW HELPER]: Initialize resolved index variable with input parameter i
            if (i < 0) { // [NEW HELPER]: Check if the provided index is negative
                resolvedIndex = static_cast<int>(matchingIndices.size()) + i; // [NEW HELPER]: Resolve negative index relative to total match count
            } // [NEW HELPER]: End of negative index evaluation block

            if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingIndices.size()) { // [NEW HELPER]: Validate bounds of resolved index
                throw std::out_of_range("Index out of bounds for type T " + errorContext + "."); // [NEW HELPER]: Throw exception if resolved index is out of bounds
            } // [NEW HELPER]: End of bounds verification conditional block

            return matchingIndices[resolvedIndex]; // [NEW HELPER]: Return the exact physical block index from the matching list
        } // [NEW HELPER]: End of resolve_block_index helper function

    } // Closes the anonymous namespace

    /**
     * @brief Saves an instance of struct T into the file using fileVector.
     */
    template<typename T>
    void save(const T& instance, const std::string& fileName) {
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

        // Delegate raw block storage and metadata indexing to fileVector
        fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
        fVec.push_back(buffer); // Push serialized byte buffer into fileVector storage
    } // End of save function

    /**
     * @brief Modifies/updates the i-th instance of type T in-place.
     * @param instance The new struct instance containing updated data.
     * @param fileName The base name of the file storage.
     * @param i Index of the instance of type T to modify (supports negative indexing, default -1 for the last instance).
     */
    template<typename T>
    void modify(const T& instance, const std::string& fileName, int i = -1) {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
        size_t blockIndexToModify = resolve_block_index<T>(fVec, i, "to modify"); // [REFACTORED]: Resolved physical block index using helper function

        // 3. Fetch the existing block to preserve its header metadata (type name, size, etc.)
        std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(blockIndexToModify)); // Fetch existing target block bytes from fileVector

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
        bool success = fVec.modify(block, static_cast<int>(blockIndexToModify)); // Call fileVector modify method with updated block
        if (!success) { // Check if the file write operation failed
            throw std::runtime_error("Failed to write modified block to fileVector storage."); // Throw error if write failed
        } // End of success check conditional block
    } // End of modify function

    /**
     * @brief Returns the total count of records matching type T in the file vector.
     */
    template<typename T>
    size_t GetSize(const std::string& fileName) {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
        return get_matching_indices<T>(fVec).size(); // [REFACTORED]: Replaced manual loop with helper function call and returned collection size
    } // End of GetSize function

    /**
     * @brief Fetches the i-th instance of type T using fileVector.
     */
    template<typename T>
    T fetch(int i, const std::string& fileName) {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
        size_t blockIndex = resolve_block_index<T>(fVec, i, "to fetch"); // [REFACTORED]: Resolved physical block index using helper function

        std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(blockIndex)); // Fetch the specific matching byte block from fileVector

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
    T Pop(const std::string& fileName, int i = -1) {
        check_plain_struct<T>(); // Validate that template parameter T is a plain struct

        fileVector fVec(fileName, fileManager); // Instantiate fileVector object bound to file manager
        size_t blockIndexToPop = resolve_block_index<T>(fVec, i, "to pop"); // [REFACTORED]: Resolved physical block index using helper function

        // Since fileVector's Pop() removes by exact vector index, if our target isn't at the end, 
        // we can fetch it, then use fileVector's Pop API or handle index removal. 
        // For efficiency, we leverage fileVector's generalized Pop method if available, 
        // or call fVec.Pop(blockIndexToPop).
        std::vector<uint8_t> poppedBlock = fVec.Pop(static_cast<int>(blockIndexToPop)); // Pop block from fileVector storage using block index

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

} // Closes the json_file namespace wrapper