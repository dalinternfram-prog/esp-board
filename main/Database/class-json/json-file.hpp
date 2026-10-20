


namespace json_file { // Begins the json_file namespace wrapper

    inline std::shared_ptr<FileManager> fileManager = std::make_shared<FileManager>(); // Shared file manager instance

    template<typename T> // Template function validation for plain structs
    void check_plain_struct() {
        static_assert(std::is_trivially_copyable_v<T>, 
            "Error: T must be a plain struct (trivially copyable).");
    }

    /**
     * @brief Saves an instance of struct T into the file using fileVector.
     */
    template<typename T>
    void save(const T& instance, const std::string& fileName) {
        check_plain_struct<T>();

        std::string typeName = typeid(T).name();
        size_t nameLen = typeName.size();
        size_t structSize = sizeof(T);

        std::vector<uint8_t> buffer;
        auto appendBytes = [&](const void* data, size_t size) {
            const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data);
            buffer.insert(buffer.end(), bytePtr, bytePtr + size);
        };

        // Serialize type metadata and payload into a single raw byte block
        appendBytes(&nameLen, sizeof(nameLen));
        appendBytes(typeName.data(), nameLen);
        appendBytes(&structSize, sizeof(structSize));
        appendBytes(&instance, structSize);

        // Delegate raw block storage and metadata indexing to fileVector
        fileVector fVec(fileName, fileManager);
        fVec.push_back(buffer);
    }

    /**
     * @brief Returns the total count of records matching type T in the file vector.
     */
    template<typename T>
    size_t GetSize(const std::string& fileName) {
        check_plain_struct<T>();

        fileVector fVec(fileName, fileManager);
        size_t totalElements = fVec.GetCurrentSize();
        if (totalElements == 0) return 0;

        size_t matchingCount = 0;
        std::string targetTypeName = typeid(T).name();

        // Iterate through stored blocks using fileVector::Fetch
        for (size_t i = 0; i < totalElements; ++i) {
            std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(i));
            if (block.size() < sizeof(size_t)) continue;

            size_t offset = 0;
            size_t nameLen;
            std::memcpy(&nameLen, &block[offset], sizeof(size_t));
            offset += sizeof(size_t);

            if (block.size() < offset + nameLen + sizeof(size_t)) continue;
            std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen);
            offset += nameLen;

            size_t structSize;
            std::memcpy(&structSize, &block[offset], sizeof(size_t));

            if (structSize == sizeof(T) && typeName == targetTypeName) {
                matchingCount++;
            }
        }

        return matchingCount;
    }

    /**
     * @brief Fetches the i-th instance of type T using fileVector.
     */
    template<typename T>
    T fetch(int i, const std::string& fileName) {
        check_plain_struct<T>();

        fileVector fVec(fileName, fileManager);
        size_t totalElements = fVec.GetCurrentSize();
        if (totalElements == 0) {
            throw std::runtime_error("File is empty or does not exist.");
        }

        std::string targetTypeName = typeid(T).name();
        int targetIndexCount = 0;

        for (size_t j = 0; j < totalElements; ++j) {
            std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(j));
            if (block.size() < sizeof(size_t)) continue;

            size_t offset = 0;
            size_t nameLen;
            std::memcpy(&nameLen, &block[offset], sizeof(size_t));
            offset += sizeof(size_t);

            if (block.size() < offset + nameLen + sizeof(size_t)) continue;
            std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen);
            offset += nameLen;

            size_t structSize;
            std::memcpy(&structSize, &block[offset], sizeof(size_t));
            offset += sizeof(size_t);

            if (structSize == sizeof(T) && typeName == targetTypeName) {
                if (targetIndexCount == i) {
                    T instance;
                    std::memcpy(&instance, &block[offset], sizeof(T));
                    return instance;
                }
                targetIndexCount++;
            }
        }

        throw std::out_of_range("Index out of bounds for type T.");
    }

    /**
     * @brief Removes and returns the i-th instance of type T (default i = -1 pops the last instance) using fileVector::Pop.
     */
    template<typename T>
    T Pop(const std::string& fileName, int i = -1) {
        check_plain_struct<T>();

        fileVector fVec(fileName, fileManager);
        size_t totalElements = fVec.GetCurrentSize();
        if (totalElements == 0) {
            throw std::runtime_error("File is empty or does not exist.");
        }

        std::string targetTypeName = typeid(T).name();
        std::vector<size_t> matchingBlockIndices;

        // Find all fileVector block indices that match type T
        for (size_t j = 0; j < totalElements; ++j) {
            std::vector<uint8_t> block = fVec.Fetch(static_cast<int>(j));
            if (block.size() < sizeof(size_t)) continue;

            size_t offset = 0;
            size_t nameLen;
            std::memcpy(&nameLen, &block[offset], sizeof(size_t));
            offset += sizeof(size_t);

            if (block.size() < offset + nameLen + sizeof(size_t)) continue;
            std::string typeName(reinterpret_cast<const char*>(&block[offset]), nameLen);
            offset += nameLen;

            size_t structSize;
            std::memcpy(&structSize, &block[offset], sizeof(size_t));

            if (structSize == sizeof(T) && typeName == targetTypeName) {
                matchingBlockIndices.push_back(j);
            }
        }

        if (matchingBlockIndices.empty()) {
            throw std::out_of_range("No instance of type T found to pop.");
        }

        // Handle negative indexing (e.g., -1 for the last element)
        int resolvedIndex = i;
        if (i < 0) {
            resolvedIndex = static_cast<int>(matchingBlockIndices.size()) + i;
        }

        if (resolvedIndex < 0 || static_cast<size_t>(resolvedIndex) >= matchingBlockIndices.size()) {
            throw std::out_of_range("Index out of bounds for type T pop.");
        }

        size_t blockIndexToPop = matchingBlockIndices[resolvedIndex];

        // Since fileVector's Pop() removes by exact vector index, if our target isn't at the end, 
        // we can fetch it, then use fileVector's Pop API or handle index removal. 
        // For efficiency, we leverage fileVector's generalized Pop method if available, 
        // or call fVec.Pop(blockIndexToPop).
        std::vector<uint8_t> poppedBlock = fVec.Pop(static_cast<int>(blockIndexToPop));

        // Extract the struct instance from the popped block bytes
        size_t offset = 0;
        size_t nameLen;
        std::memcpy(&nameLen, &poppedBlock[offset], sizeof(size_t));
        offset += sizeof(size_t) + nameLen;

        size_t structSize;
        std::memcpy(&structSize, &poppedBlock[offset], sizeof(size_t));
        offset += sizeof(size_t);

        T instance;
        std::memcpy(&instance, &poppedBlock[offset], sizeof(T));

        return instance;
    }

} // Closes the json_file namespace