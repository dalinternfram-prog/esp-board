#pragma once
#include "KcpClient.hpp"
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector> 
#include "esp_log.h"
#include "WiFiManager.hpp"

#include "file_manager.hpp"

#include "ArduinoJson.h"
#include "ArduinoJson.hpp"
namespace Network {
static const char* TAG = "KCP_CLIENT";

    class manager{

private:
bool hasWifi=false;
std::unique_ptr<KcpClient>u_KcpClient;
std::string fileName = "audio_unsend";

protected:
std::shared_ptr<WiFiManager>s_WiFiManager;
 std::shared_ptr<FileManager> fileManager;
public:


manager(uint32_t conv_id ,std::shared_ptr<WiFiManager>s_WiFiManager, std::shared_ptr<FileManager> fileManager);

 
    void close_connection()const;

    // Core Tick & Data Flow Methods
    void update(uint32_t current_ms)const;
 std::vector<uint8_t> poll_and_receive();
 
  int send(const std::vector<uint8_t> &  data);








    };
}