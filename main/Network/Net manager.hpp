#pragma once
#include "KcpClient.hpp"
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>
#include "./wifi_driver/wifi_manager.h"
#include "esp_log.h"



namespace Network {
static const char* TAG = "KCP_CLIENT";

    class manager{

private:
bool hasWifi=false;
std::unique_ptr<KcpClient>u_KcpClient;
public:


manager(uint32_t conv_id );

bool try_connectWithServer( const char* host, const char* port);
bool initWifi(const char* ssid, const char* password);

    void close_connection()const;

    // Core Tick & Data Flow Methods
    void update(uint32_t current_ms)const ;
 std::vector<uint8_t> poll_and_receive();
 
  int send(const std::vector<uint8_t> &  data);








    };
}