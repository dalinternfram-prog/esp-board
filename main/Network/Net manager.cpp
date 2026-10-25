#include "Net manager.hpp"
#include <memory>
#include <vector>
#include "encryption/interface.hpp"

namespace Network { 

    // Internal state variables (formerly class members)
  
    
    static std::unique_ptr<KcpClient> u_KcpClient;
    static std::string fileName = "packageStoredFile";
    static std::unique_ptr<file::fileVector> packageStoredFile = std::make_unique<file::fileVector>(fileName);




//TODO:this is error now,since we store un-encrypted data here;
    static void  try_send_disk_package(){
        if(!wifi::isConnected())return;
        if(!completeHandshake)return;
                // has wifi; we check the unsend one;
            const auto currentSize = packageStoredFile->GetCurrentSize();
            for(int i=0;i<currentSize;++i){
              auto unsend_ = packageStoredFile->Pop();
 
                    auto Opt_encrypted_data = encrytion::aes_gcm_encrypt(unsend_);
        if (!Opt_encrypted_data.has_value()) return  ;

        auto encrypted_data = Opt_encrypted_data.value();

                   // we encrypt here;
        int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
        if (r == -1) {
            ESP_LOGE(TAG, "send failed, reason: unknown");
        }
        return  ;

            }
 
    }
    // Initialization (replaces the constructor)
    void init(uint32_t conv_id,const char* host, const char* port) {
        // Initialize your KcpClient here, e.g.:
         u_KcpClient = std::make_unique<KcpClient>(conv_id);
        u_KcpClient->initialize(host,port);
   
   try_send_disk_package();

    }

    void close_connection() {
        if (u_KcpClient) {
            u_KcpClient->close_connection();
        }
    }

    // Core Tick & Data Flow Methods
    void update(uint32_t current_ms) {
        if (u_KcpClient) {
            u_KcpClient->update(current_ms);
        }
 
   try_send_disk_package();


    }
// TODO: this should create a watch dog to watch the information from server;
    std::vector<uint8_t> poll_and_receive() {
        if (!u_KcpClient) return {};
        
        
        auto encrypted_data = u_KcpClient->poll_and_receive();

        if(!completeHandshake){
            //* this is the server public key;
            //* we extract it ;
          std::optional<std::vector<uint8_t>> result =   encrytion::ecdh_step2_compute_shared_secret(encrypted_data);
          if(result.has_value()){
            // load nvs;
            aes_key = result.value();
            completeHandshake=true;
            return {};
          }
        }
        auto Opt_decrypted_data = encrytion::aes_gcm_decrypt(encrypted_data);

        if (!Opt_decrypted_data.has_value()) return {};
        return Opt_decrypted_data.value();
    }
// TODO: when it's first send,we don't encrypt;
    int send(const std::vector<uint8_t>& data) {



// TODO : wifi connected,but handshake haven't it;or wifi is disconnected;
if(!wifi::isConnected()||!completeHandshake){
           ESP_LOGI(TAG, "wifi disconnected,or handshake has not finished yet");
            // store in disk;
            packageStoredFile->push_back(data);
            return -1;
}
 
  
   try_send_disk_package();

        auto Opt_encrypted_data = encrytion::aes_gcm_encrypt(data);
        if (!Opt_encrypted_data.has_value()) return -1;

        auto encrypted_data = Opt_encrypted_data.value();

                   // we encrypt here;
        int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
        if (r == -1) {
            ESP_LOGI(TAG, "send failed, reason: unknown");
        }
        return r;


 
      


    }
 bool send_handshakeToServer(){
if(completeHandshake){
    
ESP_LOGE(TAG, "duplicated handshake with server");
    return false;
}

std::optional<std::vector<uint8_t>> generated_key =encrytion::ecdh_step1_generate_my_key();

if(!generated_key.has_value()){
ESP_LOGE(TAG, "!generated_key.has_value()");
    return false;
}

        int r = u_KcpClient->send(generated_key.value().data(), generated_key.value().size());
        if (r == -1) {
            ESP_LOGI(TAG, "send failed, reason: unknown");
            return false;
        }
   return true;
 
    
 }
} // namespace Network