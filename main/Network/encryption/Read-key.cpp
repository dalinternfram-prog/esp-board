






#include "interface.hpp"
#include <cstring>

static const char *TAG = "PSA_AES_GCM";

namespace encrytion {

 

 

 

// Function that loads the AES key from NVS and returns it as a std::vector inside std::optional
std::optional<std::vector<uint8_t>> load_aes_key_from_nvs() 
{
    nvs_handle_t nvs_handle;                                               // Declare an NVS handle variable
    
    // Open the NVS namespace in read-only mode since we are only fetching the key
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);         
    if (err != ESP_OK) {                                                   // Check if opening the NVS partition succeeded
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err)); // Log error if NVS open failed
        return std::nullopt;                                               // Return std::nullopt to indicate failure
    }                                                                      

    std::vector<uint8_t> key_out(AES_KEY_BYTES);                           // Allocate a vector sized to hold the exact expected key length
    size_t required_size = AES_KEY_BYTES;                                  // Set expected size variable for the key blob
    
    // Attempt to read the AES key blob from NVS storage under the key "aes_key"
    err = nvs_get_blob(nvs_handle, "aes_key", key_out.data(), &required_size);    
    
    // Always close the NVS handle immediately after your read operation is finished
    nvs_close(nvs_handle);                                                 

    if (err != ESP_OK) {                                                   // Check if the read operation was successful
        if (err == ESP_ERR_NVS_NOT_FOUND) {                                // Specifically check if the key is missing from NVS
            ESP_LOGE(TAG, "AES key does not exist in NVS!");               // Log that the key hasn't been provisioned yet
        } else {                                                           // Handle other potential NVS read errors
            ESP_LOGE(TAG, "Failed to read AES key from NVS: %s", esp_err_to_name(err)); 
        }                                                                  
        return std::nullopt;                                               // Return std::nullopt because the key could not be loaded
    }                                                                      

    // Extra safety check: verify that the retrieved key size matches expected byte length
    if (required_size != AES_KEY_BYTES) {                                  
        ESP_LOGE(TAG, "Retrieved key size mismatch: expected %d, got %u", AES_KEY_BYTES, required_size); 
        return std::nullopt;                                               // Return std::nullopt if data corruption or size mismatch occurred
    }                                                                      

    return key_out;                                                        // Return the vector containing the successfully loaded key
}




} // namespace encrytion











