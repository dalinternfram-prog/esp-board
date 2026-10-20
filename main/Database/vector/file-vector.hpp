
// #pragma once
// #include "file_manager.hpp"
// #include <string>
// #include <vector>
// #include <memory>
// #include <ArduinoJson.h>


// namespace file {

// constexpr std::string FileVectorTag = "FileVectorTag";


//  class fileVector {
// public:
//     /// @brief Structure representing the metadata of an individual stored element.
//     struct fileVectorJson {
//         size_t byteSize;    ///< Size of the element's raw byte block in bytes.
//         int offset;         ///< Byte offset where this element starts within the binary file.
//         uint32_t AddingTime; ///< Timestamp (in milliseconds) when the element was appended.
//     };

// protected:
//     std::shared_ptr<FileManager> FileManager_; ///< Shared pointer to the underlying file management utility.
//     std::string fileName{};                    ///< Base name of the target file (metadata JSON will be `fileName.json`).

// private:
// std::string readTextFile(const std::string& path);


// public:

//     /**
//      * @brief Constructs a fileVector instance, initializing or loading its accompanying JSON metadata file.
//      * @param fileName Base name of the file storage stream.
//      * @param FileManager_ Shared pointer to the FileManager instance handling low-level disk I/O.
//      */
//     fileVector(std::string fileName, std::shared_ptr<FileManager> FileManager_);


//     /**
//      * @brief Appends a new block of raw byte data to the end of the file vector.
//      * @details Automatically calculates the next byte offset, writes the raw bytes to disk, 
//      *          records the timestamp using the high-resolution timer, and updates the JSON metadata.
//      * @param data A vector of bytes (`uint8_t`) representing the element to store.
//      */
//     void push_back(const std::vector<uint8_t>& data);

//     /**
//      * @brief Retrieves the current number of elements stored in the vector.
//      * @return size_t The total count of active elements currently tracked in the metadata.
//      */
//     size_t GetCurrentSize() const;

//     /**
//      * @brief Fetches a specific element's raw byte data by its index without removing it.
//      * @details Reads only the specific byte slice using the element's offset and size to prevent RAM bloat.
//      * @param i Zero-based index of the element to fetch.
//      * @return std::vector<uint8_t> The raw byte vector of the element, or an empty vector if the index is out of bounds or reading fails.
//      */
//     std::vector<uint8_t> Fetch(int i);

//     /**
//      * @brief Removes and returns an element at a specified index using optimized compaction strategies.
//      * @details 
//      * - Supports negative indexing (e.g., `-1` targets the last element).
//      * - Popping the back (`i = -1`) uses lightweight file truncation.
//      * - Popping the front (`i = 0`) utilizes a lazy compaction threshold to minimize flash write wear.
//      * - Popping from the middle shifts subsequent bytes and adjusts metadata offsets.
//      * 
//      * @param i Index of the element to pop (supports negative indices, defaults to `-1` for back).
//      * @return std::vector<uint8_t> The removed element's raw byte data, or an empty vector on failure/out-of-bounds.
//      */
//     std::vector<uint8_t> Pop(int i = -1);


//     /**
//      * @brief Modifies the raw byte data of an existing element at the specified index.
//      * 
//      * @param data The new byte data to write. Must be identical in size to the existing element.
//      * @param i The index of the element to modify (supports negative indices, e.g., -1 for the last element).
//      * @return true if the modification succeeded, or false if the index is out of bounds, 
//      *         the file manager is unavailable, or the new data size does not match the existing element's size.
//      */
//     bool modify(const std::vector<uint8_t>& data, int i=-1);
// };
// }















#pragma once
#include "file_manager.hpp"
#include <string>
#include <vector>
#include <ArduinoJson.h>


namespace file {

constexpr std::string FileVectorTag = "FileVectorTag";


 class fileVector {
public:
    /// @brief Structure representing the metadata of an individual stored element.
    struct fileVectorJson {
        size_t byteSize;    ///< Size of the element's raw byte block in bytes.
        int offset;         ///< Byte offset where this element starts within the binary file.
        uint32_t AddingTime; ///< Timestamp (in milliseconds) when the element was appended.
    };

protected:
    std::string fileName{};                    ///< Base name of the target file (metadata JSON will be `fileName.json`).

private:
std::string readTextFile(const std::string& path)const;


public:

    /**
     * @brief Constructs a fileVector instance, initializing or loading its accompanying JSON metadata file.
     * @param fileName Base name of the file storage stream.
     */
    fileVector(std::string fileName);


    /**
     * @brief Appends a new block of raw byte data to the end of the file vector.
     * @details Automatically calculates the next byte offset, writes the raw bytes to disk, 
     *          records the timestamp using the high-resolution timer, and updates the JSON metadata.
     * @param data A vector of bytes (`uint8_t`) representing the element to store.
     */
    void push_back(const std::vector<uint8_t>& data);

    /**
     * @brief Retrieves the current number of elements stored in the vector.
     * @return size_t The total count of active elements currently tracked in the metadata.
     */
    size_t GetCurrentSize() const;

    /**
     * @brief Fetches a specific element's raw byte data by its index without removing it.
     * @details Reads only the specific byte slice using the element's offset and size to prevent RAM bloat.
     * @param i Zero-based index of the element to fetch.
     * @return std::vector<uint8_t> The raw byte vector of the element, or an empty vector if the index is out of bounds or reading fails.
     */
    std::vector<uint8_t> Fetch(int i)const;;

    /**
     * @brief Removes and returns an element at a specified index using optimized compaction strategies.
     * @details 
     * - Supports negative indexing (e.g., `-1` targets the last element).
     * - Popping the back (`i = -1`) uses lightweight file truncation.
     * - Popping the front (`i = 0`) utilizes a lazy compaction threshold to minimize flash write wear.
     * - Popping from the middle shifts subsequent bytes and adjusts metadata offsets.
     * 
     * @param i Index of the element to pop (supports negative indices, defaults to `-1` for back).
     * @return std::vector<uint8_t> The removed element's raw byte data, or an empty vector on failure/out-of-bounds.
     */
    std::vector<uint8_t> Pop(int i = -1);


    /**
     * @brief Modifies the raw byte data of an existing element at the specified index.
     * 
     * @param data The new byte data to write. Must be identical in size to the existing element.
     * @param i The index of the element to modify (supports negative indices, e.g., -1 for the last element).
     * @return true if the modification succeeded, or false if the index is out of bounds, 
     *         the file manager is unavailable, or the new data size does not match the existing element's size.
     */
    bool modify(const std::vector<uint8_t>& data, int i=-1);
};
}