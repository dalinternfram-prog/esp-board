 
#include "./Net manager.hpp"
#include <vector>
#include "encryption/interface.hpp"
namespace Network { 
 

manager::manager(uint32_t conv_id ){
    this->u_KcpClient=std::make_unique<KcpClient >(conv_id);
}


bool manager::try_connectWithServer( const char* host, const char* port){
    bool success = this->u_KcpClient-> initialize(host,port);
    if(!success)
    {
        ESP_LOGI(TAG,"host port init failed");
       
    }

    return success==true;
}
bool manager::initWifi(const char* ssid, const char* password){
    if(wifi_manager_init_sta(ssid,password)==ESP_OK)this->hasWifi=true;
    return hasWifi; 
}

    void manager::close_connection()const {
        this->u_KcpClient->close_connection();
    }


    // Core Tick & Data Flow Methods
    void manager::update(uint32_t current_ms)const {
        this->u_KcpClient->update(current_ms);
    }

 std::vector<uint8_t> manager::poll_and_receive(){

    auto encrypted_data = this->u_KcpClient->poll_and_receive();

    auto Opt_decrypted_data = encrytion::aes_gcm_decrypt(encrypted_data);


if(!Opt_decrypted_data.has_value())return {};
return Opt_decrypted_data.value();

 
 }



    int manager::send(const std::vector<uint8_t> &  data){
        if(!wifi_manager_is_connected())ESP_LOGI(TAG,"wifi disconnected");
        return -1;



auto Opt_encrypted_data = encrytion::aes_gcm_encrypt(data);
if(!Opt_encrypted_data.has_value())return -1;

auto encrypted_data = Opt_encrypted_data.value();


        /// we encrypt here;
        int r = this->u_KcpClient->send(encrypted_data.data(),encrypted_data.size());
        if(r==-1)ESP_LOGI(TAG,"send failed,reason:unknow");


        return r;
    }

 
}