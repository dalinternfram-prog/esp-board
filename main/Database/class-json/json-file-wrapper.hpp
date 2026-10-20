 
// // #pragma once
// // #include "file_manager.hpp"
// // #include <memory>
// // #include <string>
// // #include <vector>
// // #include <type_traits>
// // #include <cstring>
// // #include <stdexcept>
// // #include <typeinfo>

// // namespace json_file { // Begins the json_file namespace wrapper

// // class JsonFileClass {
// // private:
// //     std::shared_ptr<FileManager> fileManager_; // Shared file manager instance
// //     std::unique_ptr<FileManager>  JsonFileStore_; 



// // public:
// //     // [CHANGED]: Updated constructor parameter order to match fileVector(std::string fileName, std::shared_ptr<FileManager> FileManager_)
// //     /**
// //      * @brief Constructs a JsonFileStore instance, initializing or loading its accompanying JSON metadata file.
// //      * @param fileName Base name of the file storage stream.
// //      * @param fileManager Shared pointer to the FileManager instance handling low-level disk I/O.
// //      */
// //     JsonFileStore(const std::string& fileName, std::shared_ptr<FileManager> fileManager)
// //         : fileManager_(std::move(fileManager)), fVec_(fileName, fileManager_) {} // [CHANGED]: Initialized fVec_ matching fileVector signature order

// //     // [CHANGED]: Added convenience constructor that initializes a default FileManager internally while keeping fileName first
// //     /**
// //      * @brief Constructs a JsonFileStore instance with a newly created default FileManager.
// //      * @param fileName Base name of the file storage stream.
// //      */
// //     explicit JsonFileStore(const std::string& fileName)
// //         : fileManager_(std::make_shared<FileManager>()), fVec_(fileName, fileManager_) {}

// //     /**
// //      * @brief Saves an instance of struct T into the file using fileVector.
// //      */
// //     template<typename T>
// //     void save(const T& instance);

// //     /**
// //      * @brief Modifies/updates the i-th instance of type T in-place.
// //      * @param instance The new struct instance containing updated data.
// //      * @param i Index of the instance of type T to modify (supports negative indexing, default -1 for the last instance).
// //      */
// //     template<typename T>
// //     void modify(const T& instance, int i = -1);

// //     /**
// //      * @brief Returns the total count of records matching type T in the file vector.
// //      */
// //     template<typename T>
// //     size_t GetSize() const ;

// //     /**
// //      * @brief Fetches the i-th instance of type T using fileVector.
// //      */
// //     template<typename T>
// //     T fetch(int i) const ;

// //     /**
// //      * @brief Removes and returns the i-th instance of type T (default i = -1 pops the last instance) using fileVector::Pop.
// //      */
// //     template<typename T>
// //     T Pop(int i = -1) ;

// // } // Closes the JsonFileStore class

// // } // Closes the json_file namespace wrapper














// #pragma once
// #include "json-file.hpp" // Assuming JsonFileStore is defined here or included
// #include <memory>
// #include <string>
// #include <vector>
// #include <source_location>
// #include <filesystem>

// namespace json_file { // Begins the json_file namespace wrapper

// class JsonFileClass {
// private:
//     JsonFileStore store_; // Backing JsonFileStore instance handling the data operations

//     // Automatically derives a unique storage filename from the caller's source location context
//     static std::string derive_filename_from_caller(const std::source_location& loc) {
//         std::filesystem::path p(loc.file_name());
//         std::string baseName = p.stem().string(); // e.g., extracts "main" from "/path/to/main.cpp"
        
//         // Combine filename with a hash of the function/class scope to ensure uniqueness per scope
//         return baseName + "_" + std::to_string(std::hash<std::string>{}(loc.function_name())) + ".bin";
//     }

// public:
//     /**
//      * @brief Constructs a JsonFileClass instance, automatically extracting the caller's 
//      *        file and scope context to hide filename management, and delegating to JsonFileStore.
//      * @param fileManager Shared pointer to the FileManager instance handling low-level disk I/O.
//      * @param loc Automatically captured source location of the caller (C++20).
//      */
//     explicit JsonFileClass(std::shared_ptr<FileManager> fileManager = std::make_shared<FileManager>(),
//                            const std::source_location& loc = std::source_location::current())
//         : store_(derive_filename_from_caller(loc), std::move(fileManager)) {}

//     /**
//      * @brief Saves an instance of struct T into the file using JsonFileStore.
//      */
//     template<typename T>
//     void save(const T& instance) {
//         store_.save(instance);
//     }

//     /**
//      * @brief Modifies/updates the i-th instance of type T in-place using JsonFileStore.
//      */
//     template<typename T>
//     void modify(const T& instance, int i = -1) {
//         store_.modify(instance, i);
//     }

//     /**
//      * @brief Returns the total count of records matching type T using JsonFileStore.
//      */
//     template<typename T>
//     size_t GetSize() const {
//         return store_.GetSize<T>();
//     }

//     /**
//      * @brief Fetches the i-th instance of type T using JsonFileStore.
//      */
//     template<typename T>
//     T fetch(int i) const {
//         return store_.fetch<T>(i);
//     }

//     /**
//      * @brief Removes and returns the i-th instance of type T using JsonFileStore.
//      */
//     template<typename T>
//     T Pop(int i = -1) {
//         return store_.Pop<T>(i);
//     }

// }; // Closes the JsonFileClass class

// } // Closes the json_file namespace wrapper