 
#include "Net manager.hpp"
#include <vector>
#include "encryption/interface.hpp"
namespace Network { 
 

manager::manager(uint32_t conv_id ,std::shared_ptr<WiFiManager>s_WiFiManager, std::shared_ptr<FileManager> fileManager):s_WiFiManager(s_WiFiManager), fileManager(fileManager){
    this->u_KcpClient=std::make_unique<KcpClient >(conv_id);
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







auto Opt_encrypted_data = encrytion::aes_gcm_encrypt(data);
if(!Opt_encrypted_data.has_value())return -1;





auto encrypted_data = Opt_encrypted_data.value();

        if(!s_WiFiManager->isConnected())
        {


                ESP_LOGI(TAG,"wifi disconnected");
             // store in disk;
              fileManager->appendFile(this->fileName,encrypted_data);


        return -1;
        }
    

        /// we encrypt here;
        int r = this->u_KcpClient->send(encrypted_data.data(),encrypted_data.size());
        if(r==-1)ESP_LOGI(TAG,"send failed,reason:unknow");


        return r;
    }

 
}