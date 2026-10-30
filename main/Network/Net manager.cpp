#include "Net manager.hpp"
#include <memory>
#include <vector>
#include "encryption/interface.hpp"

namespace Network { 

    // Internal state variables (formerly class members)
  
    
    static std::unique_ptr<KcpClient> u_KcpClient;
    static std::string fileName = "packageStoredFile";
    static std::unique_ptr<file::fileVector> packageStoredFile = std::make_unique<file::fileVector>(fileName);
    static uint32_t conv_id; 


//TODO:this is error now,since we store un-encrypted data here;
    static void  try_send_disk_package(){
        if(!wifi::isConnected())return;
        if(!completeHandshake)return;
                // has wifi; we check the unsend one;
            const auto currentSize = packageStoredFile->GetCurrentSize();
            for(int i=0;i<currentSize;++i){
              auto unsend_ = packageStoredFile->Pop();
 
                    auto Opt_encrypted_data = encryption::aes_gcm_encrypt(unsend_,aes_key);
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
    void init( const char* host, const char* port) {
         conv_id = 0;
 
        conv_id = esp_random();
 
        // Initialize your KcpClient here, e.g.:
         u_KcpClient = std::make_unique<KcpClient>(conv_id);
        u_KcpClient->initialize(host,port);
   
   //try_send_disk_package();

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
 
  // try_send_disk_package();


    }
// TODO: this should create a watch dog to watch the information from server;
    std::vector<uint8_t> poll_and_receive() {
        if (!u_KcpClient) return {};
        
        
        auto encrypted_data = u_KcpClient->poll_and_receive();
          if(encrypted_data.size()==0)return {};


        if(!completeHandshake){
            //* this is the server public key;
            //* we extract it ;
          std::optional<std::vector<uint8_t>> result =   encryption::ecdh_step2_compute_shared_secret(encrypted_data);
          if(result.has_value()){
             
            aes_key = result.value();
            completeHandshake=true;

            // --- ADD THIS CODE TO PRINT THE KEY AS HEX ---
            std::string hex_str = "";
            for (uint8_t b : aes_key) {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02x", b);
                hex_str += buf;
            }
            ESP_LOGE(TAG, "Client Shared Secret (Hex): %s", hex_str.c_str());


            // ---------------------------------------------
            return {};
          }
        }
        auto Opt_decrypted_data = encryption::aes_gcm_decrypt(encrypted_data);

        if (!Opt_decrypted_data.has_value()) return {};
        return Opt_decrypted_data.value();
    }


    






// TODO: when it's first send,we don't encrypt;
    int send(const std::vector<uint8_t>& data) {

        //TODO test;
if(!wifi::isConnected()){
     ESP_LOGI(TAG, "wifi disconnected ");
            // store in disk;
         //   packageStoredFile->push_back(data);
            return -1;
}
 
//Todo test;
if( !completeHandshake){
           //ESP_LOGI(TAG, "  handshake has not finished yet");
            // store in disk;
          //  packageStoredFile->push_back(data);
            return -1;
}
 
  
  // try_send_disk_package();

         //  ESP_LOGW(TAG, " pay load size = %u bytes",data.size());
        auto Opt_encrypted_data = encryption::aes_gcm_encrypt(data,aes_key);
        if (!Opt_encrypted_data.has_value()) return -1;

        auto encrypted_data = Opt_encrypted_data.value();

                   // we encrypt here;

                   //TODO: test;
        int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
        if (r == -1) {
            ESP_LOGI(TAG, "send failed, reason: unknown");
        }
        return r;
    }
 bool send_handshakeToServer(){
if (completeHandshake) {
            ESP_LOGE(TAG, "duplicated handshake with server");
            return false;
        }

        std::optional<std::vector<uint8_t>> generated_key = encryption::ecdh_step1_generate_my_key();
        if (!generated_key.has_value()) {
            ESP_LOGE(TAG, "!generated_key.has_value()");
            return false;
        }

        const auto& pub_key_bytes = generated_key.value();
        std::vector<uint8_t> handshake_packet;
        handshake_packet.resize(sizeof(uint32_t) + pub_key_bytes.size());

        // Copy conv_id into the first 4 bytes
        std::memcpy(handshake_packet.data(), &conv_id, sizeof(uint32_t));

        // Copy public key bytes right after it
        std::memcpy(handshake_packet.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

        ESP_LOGW(TAG, "ready to send to server the handshake (conv_id: %u)", conv_id);

        // FIX 2: Send the full handshake_packet instead of just the raw key
        int r = u_KcpClient->raw_udp_send(handshake_packet.data(), handshake_packet.size());
        if (r == -1) {
            ESP_LOGE(TAG, "send failed, reason: unknown");
            return false;
        }
        return true;
    }


} // namespace Network