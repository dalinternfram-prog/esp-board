

namespace json_file { // Begins the json_file namespace wrapper

    inline FileManager fileManager; // Declares a shared or global inline instance of FileManager

    template<typename T> // Template function declaration to validate struct properties at compile time
    void check_plain_struct() { // Defines the helper function for type checking
        static_assert(std::is_trivially_copyable_v<T>, // Compile-time check ensuring T is a plain, trivially copyable struct
            "Error: T must be a plain struct (trivially copyable)."); // Compilation error message if check fails
    } // Closes check_plain_struct function

    template<typename T> // Template function declaration for saving struct instances
    void save(const T& instance, const std::string& fileName) { // Fixed T&& to const T& to prevent reference deduction bugs
        check_plain_struct<T>(); // Calls the static type check for type T

        std::string typeName = typeid(T).name(); // Retrieves the platform-dependent mangled type name string
        size_t nameLen = typeName.size(); // Calculates and stores the length of the type name string
        size_t structSize = sizeof(T); // Stores the exact byte size of the struct T

        std::vector<uint8_t> buffer; // Initializes a dynamic byte buffer vector for serialization
        
        auto appendBytes = [&](const void* data, size_t size) { // Lambda helper function to safely append raw bytes to the buffer
            const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data); // Casts input pointer to a byte pointer
            buffer.insert(buffer.end(), bytePtr, bytePtr + size); // Inserts the byte block into the end of the buffer vector
        }; // Closes the lambda definition

        appendBytes(&nameLen, sizeof(nameLen)); // Serializes and appends the type name length header
        appendBytes(typeName.data(), nameLen); // Serializes and appends the actual type name characters
        appendBytes(&structSize, sizeof(structSize)); // Serializes and appends the struct size header
        appendBytes(&instance, structSize); // Serializes and appends the raw byte contents of the struct instance

        fileManager.appendFile(fileName, buffer); // Sends the completed byte buffer to the file manager to append
    } // Closes the save function

 // 2. FETCH: Reads the file via FileManager, parses records, finds the i-th instance of type T
    template<typename T>
    T fetch(int i, const std::string& fileName) {
        check_plain_struct<T>();

        std::vector<uint8_t> fileBytes = fileManager.readFileBytes(fileName);
        if (fileBytes.empty()) {
            throw std::runtime_error("File is empty or does not exist.");
        }

        size_t offset = 0;
        int targetIndexCount = 0; // Tracks the specific index count for type T only

        while (offset < fileBytes.size()) {
            if (offset + sizeof(size_t) > fileBytes.size()) break;
            
            size_t nameLen;
            std::memcpy(&nameLen, &fileBytes[offset], sizeof(size_t));
            offset += sizeof(size_t);

            if (offset + nameLen > fileBytes.size()) break;
            std::string typeName(reinterpret_cast<const char*>(&fileBytes[offset]), nameLen);
            offset += nameLen;

            if (offset + sizeof(size_t) > fileBytes.size()) break;
            size_t structSize;
            std::memcpy(&structSize, &fileBytes[offset], sizeof(size_t));
            offset += sizeof(size_t);

            if (offset + structSize > fileBytes.size()) break;

            // Check if the current record matches the requested type T
            if (structSize == sizeof(T) && typeName == typeid(T).name()) {
                if (targetIndexCount == i) {
                    T instance;
                    std::memcpy(&instance, &fileBytes[offset], sizeof(T));
                    return instance;
                }
                targetIndexCount++; // Only increment when we find a valid instance of type T
            }

            // Always advance offset past the current struct payload to check the next record
            offset += structSize;
        }

        throw std::out_of_range("Index out of bounds for type T.");
    }
    template<typename T> // Template function declaration to count matching records in a file
    size_t GetSize(const std::string& fileName) { // Defines GetSize function returning total count of records of type T
        check_plain_struct<T>(); // Calls the static type check for type T

        std::vector<uint8_t> fileBytes = fileManager.readFileBytes(fileName); // Reads the file contents into a byte vector
        if (fileBytes.empty()) return 0; // Returns zero immediately if the file is empty or non-existent

        size_t offset = 0; // Initializes the byte stream parsing offset tracker
        size_t count = 0; // Initializes the counter tracking matching records of type T

        while (offset < fileBytes.size()) { // Loops through the byte buffer until the end of the file
            if (offset + sizeof(size_t) > fileBytes.size()) break; // Bounds check for reading name length header
            size_t nameLen; // Declares local variable for name length
            std::memcpy(&nameLen, &fileBytes[offset], sizeof(size_t)); // Safely copies nameLen avoiding unaligned pointer casting
            offset += sizeof(size_t); // Advances offset past name length header

            if (offset + nameLen > fileBytes.size()) break; // Bounds check for reading type name string
            std::string typeName(reinterpret_cast<const char*>(&fileBytes[offset]), nameLen); // Reads the type name string safely
            offset += nameLen; // Advances offset past type name string

            if (offset + sizeof(size_t) > fileBytes.size()) break; // Bounds check for reading struct size header
            size_t structSize; // Declares local variable for struct size
            std::memcpy(&structSize, &fileBytes[offset], sizeof(size_t)); // Safely copies structSize avoiding unaligned pointer casting
            offset += sizeof(size_t); // Advances offset past struct size header

            if (offset + structSize > fileBytes.size()) break; // Bounds check for reading struct payload bytes

            if (structSize == sizeof(T) && typeName == typeid(T).name()) { // Checks if record size and type name match type T
                count++; // Increments the matching record counter
            } // Closes match validation check

            offset += structSize; // Advances offset past the current struct payload to the next record
        } // Closes the while parsing loop

        return count; // Returns the total accumulated count of matching records
    } // Closes the GetSize function


 // Generalized POP: Removes and returns the i-th instance of type T (default i = -1 pops the last instance / LIFO)
    template<typename T>
    T Pop(const std::string& fileName, int i = -1) { // Uses int i so -1 can safely represent the default "last element" behavior
        check_plain_struct<T>(); // Validates that T is a trivially copyable plain struct at compile time

        std::vector<uint8_t> fileBytes = fileManager.readFileBytes(fileName); // Reads the entire file into a byte vector
        if (fileBytes.empty()) { // Checks if the file is empty or does not exist
            throw std::runtime_error("File is empty or does not exist."); // Throws an error if no data is present
        } // Closes file empty check

        struct RecordInfo { // Internal helper structure to track each record's position and type metadata
            size_t startOffset; // Byte offset where the record begins
            size_t totalRecordSize; // Total size of the record header + payload in bytes
            bool matchesTarget; // Flag indicating if this record matches type T
        }; // Closes RecordInfo definition

        std::vector<RecordInfo> records; // Vector to store metadata for all parsed records in the file
        size_t offset = 0; // Initializes the byte stream parsing offset tracker
        int targetIndexCount = 0; // Tracks the specific index count for type T matches
        int targetRecordIndexToPop = -1; // Stores the absolute index in the records vector of the item to remove

        // First pass: Parse and index all records in the file
        while (offset < fileBytes.size()) { // Loops through the byte buffer until the end of the file
            size_t recordStart = offset; // Records the starting offset of the current item

            if (offset + sizeof(size_t) > fileBytes.size()) break; // Bounds check for name length header
            size_t nameLen; // Declares local variable for name length
            std::memcpy(&nameLen, &fileBytes[offset], sizeof(size_t)); // Safely copies nameLen avoiding unaligned access
            offset += sizeof(size_t); // Advances offset past the name length header

            if (offset + nameLen > fileBytes.size()) break; // Bounds check for type name string
            std::string typeName(reinterpret_cast<const char*>(&fileBytes[offset]), nameLen); // Extracts the type name string
            offset += nameLen; // Advances offset past the type name string

            if (offset + sizeof(size_t) > fileBytes.size()) break; // Bounds check for struct size header
            size_t structSize; // Declares local variable for struct size
            std::memcpy(&structSize, &fileBytes[offset], sizeof(size_t)); // Safely copies structSize avoiding unaligned access
            offset += sizeof(size_t); // Advances offset past struct size header

            if (offset + structSize > fileBytes.size()) break; // Bounds check for struct payload bytes
            
            bool isMatch = (structSize == sizeof(T) && typeName == typeid(T).name()); // Checks if record matches type T
            size_t totalSize = (offset - recordStart) + structSize; // Calculates total bytes occupied by this record

            size_t currentRecordVectorIndex = records.size(); // Gets the index this record will have in our vector
            records.push_back({recordStart, totalSize, isMatch}); // Stores the metadata for this record

            if (isMatch) { // If this record matches type T
                // Handle default parameter i = -1 (meaning pop the very last instance)
                int targetIndexToFind = i;
                if (i < 0) {
                    // We need to know the total count first or track candidates. 
                    // To handle negative indexing smoothly, let's locate matching indices first or resolve below.
                }
                targetIndexCount++; // Increments the count of type T matches found so far
            } // Closes match check

            offset += structSize; // Advances offset past the current struct payload to the next record
        } // Closes parsing loop

        // Collect all vector indices that match type T
        std::vector<size_t> matchingRecordIndices; // Vector to hold positions of all matching T records
        for (size_t idx = 0; idx < records.size(); ++idx) { // Loops through all parsed record metadata
            if (records[idx].matchesTarget) { // Checks if the record is of type T
                matchingRecordIndices.push_back(idx); // Records its position
            } // Closes match check
        } // Closes collection loop

        if (matchingRecordIndices.empty()) { // Checks if zero instances of type T exist in the file
            throw std::out_of_range("No instance of type T found to pop."); // Throws out of range exception
        } // Closes empty check

        // Resolve target index if negative (e.g., -1 means last, -2 means second-to-last, etc.)
        int resolvedIndex = i;
        if (i < 0) {
            resolvedIndex = static_cast<int>(matchingRecordIndices.size()) + i; // Converts negative index to positive (e.g., size - 1 for -1)
        } // Closes negative index resolution

        // Validate that the resolved index is within bounds for type T
        if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingRecordIndices.size()) {
            throw std::out_of_range("Index out of bounds for type T pop."); // Throws out of bounds if index is invalid
        } // Closes bounds check

        size_t vectorIndexToPop = matchingRecordIndices[resolvedIndex]; // Gets the exact vector index of the record to remove
        const RecordInfo& targetRecord = records[vectorIndexToPop]; // Retrieves the metadata for the target record

        // Extract the target instance data before removing it
        size_t payloadOffset = targetRecord.startOffset + sizeof(size_t) + 
                               *reinterpret_cast<const size_t*>(&fileBytes[targetRecord.startOffset]) + 
                               sizeof(size_t); // Computes exact byte position of the struct payload
        
        T instance; // Declares local instance of struct T
        std::memcpy(&instance, &fileBytes[payloadOffset], sizeof(T)); // Copies raw bytes into the instance

        // Rebuild a new byte buffer excluding only the target record
        std::vector<uint8_t> newFileBytes; // Initializes buffer for reconstructed file data
        for (size_t idx = 0; idx < records.size(); ++idx) { // Loops through all records again
            if (idx == vectorIndexToPop) continue; // Skips writing the target record to omit it
            
            // Appends all bytes belonging to non-target records into the new buffer
            newFileBytes.insert(newFileBytes.end(), 
                                &fileBytes[records[idx].startOffset], 
                                &fileBytes[records[idx].startOffset + records[idx].totalRecordSize]);
        } // Closes reconstruction loop

        fileManager.writeFile(fileName, newFileBytes); // Overwrites the file with the updated byte buffer
        return instance; // Returns the popped struct instance
    } // Closes generalized Pop function



} // Closes the json_file namespace