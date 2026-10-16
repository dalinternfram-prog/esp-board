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


manager(uint32_t conv_id ){
    this->u_KcpClient=std::make_unique<KcpClient >(conv_id);
}


bool try_connectWithServer( const char* host, const char* port){
    bool success = this->u_KcpClient-> initialize(host,port);
    if(!success)
    {
        ESP_LOGI(TAG,"host port init failed");
       
    }

    return success==true;
}
bool initWifi(const char* ssid, const char* password){
    if(wifi_manager_init_sta(ssid,password)==ESP_OK)this->hasWifi=true;
    return hasWifi; 
}

    void close_connection()const {
        this->u_KcpClient->close_connection();
    }


    // Core Tick & Data Flow Methods
    void update(uint32_t current_ms)const {
        this->u_KcpClient->update(current_ms);
    }

 std::vector<uint8_t> poll_and_receive(){
    return this->u_KcpClient->poll_and_receive();
 }

    int send(const uint8_t* data, size_t size){
        if(!wifi_manager_is_connected())ESP_LOGI(TAG,"wifi disconnected");
        return -1;

        int r = this->u_KcpClient->send(data,size);
        if(r==-1)ESP_LOGI(TAG,"send failed,reason:unknow");


        return r;
    }










    };
}