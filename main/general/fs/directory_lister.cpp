#include "general/fs/directory_lister.hpp"

#include <cstring>
#include <dirent.h>
#include <strings.h>

namespace general::fs {

std::vector<std::string> listFiles(const std::string& directory,
                                   const std::string& extension,
                                   std::size_t max_files) {
    std::vector<std::string> files;

    DIR* dir = ::opendir(directory.c_str());
    if (dir == nullptr) {
        return files;
    }

    struct dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        if (max_files != 0 && files.size() >= max_files) {
            break;
        }
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        const char* dot = std::strrchr(entry->d_name, '.');
        if (dot == nullptr || dot == entry->d_name) {
            continue;
        }
        if (::strcasecmp(dot, extension.c_str()) != 0) {
            continue;
        }

        files.emplace_back(entry->d_name);
    }

    ::closedir(dir);
    return files;
}

} // namespace general::fs
