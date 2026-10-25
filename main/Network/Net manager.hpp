#pragma once
#include "KcpClient.hpp"
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector> 
#include "esp_log.h"
#include "WiFiManager.hpp"
#include "file_manager.hpp"
#include "file-vector.hpp"
#include "ArduinoJson.h"
#include "ArduinoJson.hpp"

namespace Network {
    inline const char* TAG = "KCP_CLIENT";
  inline  bool completeHandshake =false;
    // Initialization & Lifecycle
    void init(uint32_t conv_id,const char* host, const char* port);
    void close_connection();
inline std::vector<uint8_t>aes_key;

    // Core Tick & Data Flow Methods
    void update(uint32_t current_ms);
    std::vector<uint8_t> poll_and_receive();
    int send(const std::vector<uint8_t>& data);
    bool send_handshakeToServer();
    
} // namespace Network