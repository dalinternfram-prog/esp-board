#pragma once
#include "esp_spiffs.h"
#include <string>
#include "esp_err.h"
#include <vector>
class FileManager {
public:
    FileManager();
    ~FileManager();

    esp_err_t init(const std::string& base_path = "/spiffs", const std::string& partition_label = "my_files");
    esp_err_t deinit();

    bool writeFile(const std::string& filename, const std::string& data);
    bool appendFile(const std::string& filename, const std::string& data);   
     bool writeFile(const std::string& filename, const std::vector<uint8_t>& data);
    bool appendFile(const std::string& filename, const std::vector<uint8_t>& data);
    std::string readFile(const std::string& filename);
    std::vector<uint8_t> readFileBytes(const std::string& filename);
    std::vector<uint8_t> readFileBytes(const std::string& filename, size_t offset, size_t size);
    bool deleteFile(const std::string& filename);
    bool exists(const std::string& filename);
    bool getStorageStats(size_t* total_bytes, size_t* free_bytes);
void shiftBytes(const std::string& filename, size_t shiftDistance, bool left);
private:
    std::string m_base_path;
    std::string m_partition_label;
    bool m_is_mounted;
    std::string getFullPath(const std::string& filename) const;
};