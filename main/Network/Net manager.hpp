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

    // Initialization & Lifecycle
    void init(uint32_t conv_id);
    void close_connection();

    // Core Tick & Data Flow Methods
    void update(uint32_t current_ms);
    std::vector<uint8_t> poll_and_receive();
    int send(const std::vector<uint8_t>& data);

} // namespace Network