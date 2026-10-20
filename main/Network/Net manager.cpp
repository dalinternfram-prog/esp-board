#include "Net manager.hpp"
#include <memory>
#include <vector>
#include "encryption/interface.hpp"

namespace Network { 

    // Internal state variables (formerly class members)
  
    
    static std::unique_ptr<KcpClient> u_KcpClient;
    static std::string fileName = "packageStoredFile";
    static std::unique_ptr<file::fileVector> packageStoredFile = std::make_unique<file::fileVector>(fileName);





    static void  try_send_disk_package(){
        if(!wifi::isConnected())return;
                // has wifi; we check the unsend one;
            const auto currentSize = packageStoredFile->GetCurrentSize();
            for(int i=0;i<currentSize;++i){
              auto unsend_ = packageStoredFile->Pop();
             int r = u_KcpClient->send(unsend_.data(), unsend_.size());
            if (r == -1) {
            ESP_LOGE(TAG, "send failed, reason: unknown");
            // restart the board;
             }
            }
 
    }
    // Initialization (replaces the constructor)
    void init(uint32_t conv_id) {
        // Initialize your KcpClient here, e.g.:
         u_KcpClient = std::make_unique<KcpClient>(conv_id);
        
   
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

    std::vector<uint8_t> poll_and_receive() {
        if (!u_KcpClient) return {};
        
   try_send_disk_package();
        auto encrypted_data = u_KcpClient->poll_and_receive();
        auto Opt_decrypted_data = encrytion::aes_gcm_decrypt(encrypted_data);

        if (!Opt_decrypted_data.has_value()) return {};
        return Opt_decrypted_data.value();
    }

    int send(const std::vector<uint8_t>& data) {

        


        auto Opt_encrypted_data = encrytion::aes_gcm_encrypt(data);
        if (!Opt_encrypted_data.has_value()) return -1;

        auto encrypted_data = Opt_encrypted_data.value();

        if (wifi::isConnected()) {
            
   try_send_disk_package();
                   // we encrypt here;
        int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
        if (r == -1) {
            ESP_LOGI(TAG, "send failed, reason: unknown");
        }
        return r;
        }

        else{
          ESP_LOGI(TAG, "wifi disconnected");
            // store in disk;
            packageStoredFile->push_back(encrypted_data);
            return -1;
        }
      


    }

} // namespace Network