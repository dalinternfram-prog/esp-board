#pragma once






 #include<string>
#include "esp_wifi_types_generic.h"
#include "wifi/wifi-callback/wifi-state.hpp"
#include "wifi_client.hpp"
#include "wifi_init.hpp"
#include "wifi_provision.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



namespace wifi::try_connect {



static const char* TAG = "AppLoop";
static constexpr int MaxWifiNum = 33; // Define maximum stored Wi-Fi slots limit


// Structure to pass required modules into the background monitor task safely
struct WifiMonitorParams {
    std::shared_ptr<wifi_state> wifi_state_;
    std::shared_ptr<NvsManager> nvsManager;
    std::shared_ptr<WifiProvision> wifiProvision;
};




// [MODULAR] Helper function to handle connection attempt + stabilization polling + fast-fail
inline void  tryConnectAndStabilize(  const std::string& ssid, const std::string& password) {
 
        wifi_config_t wifi_config = {}; // Initialize Wi-Fi configuration structure with zeroes
    std::strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid)); // Copy SSID safely
    std::strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password)); // Copy Password safely
    wifi::switch_wifi_mode(wifi_mode_t::WIFI_MODE_STA,&wifi_config);


    return  ;
}

// [MODULAR] Handle user provisioning input
inline bool handleProvisioning(WifiMonitorParams* params, bool& provisioning_active) {
    if (!params->wifiProvision->hasCredentials()) {
        return false;
    }

    std::string new_ssid = params->wifiProvision->getSsid();
    std::string new_pass = params->wifiProvision->getPassword();
    
    // Immediately clear/consume so it doesn't loop on the same bad input forever!
    params->wifiProvision->clearCredentials();

params->wifiProvision->stop();


tryConnectAndStabilize(params->wifi_state_, new_ssid, new_pass)
    if () {
        ESP_LOGI(TAG, "Provisioned connection successful! Saving to NVS...");

        // Find first available empty slot or fallback to last slot
        int target_slot = 0;
        for (int k = 0; k < MaxWifiNum; ++k) {
            std::string slot_key = "wifi_" + std::to_string(k) + "_ssid";
            if (!params->nvsManager->exists(slot_key)) {
                target_slot = k;
                break;
            }
            if (k == MaxWifiNum - 1) {
                target_slot = MaxWifiNum - 1;
            }
        }

        params->nvsManager->writeFile("wifi_" + std::to_string(target_slot) + "_ssid", new_ssid);
        params->nvsManager->writeFile("wifi_" + std::to_string(target_slot) + "_pass", new_pass);

        params->wifiProvision->stop();
        provisioning_active = false;
        return true;
    }

    return false;
}

// [MODULAR] Handle linear NVS slot scanning
inline bool handleNvsSlots(WifiMonitorParams* params) {
    for (int i = 0; i < MaxWifiNum; ++i) {
        // Re-check connection status on the fly
        if (params->wifiClient->isConnected()) {
            return true;
        }

        std::string ssid_key = "wifi_" + std::to_string(i) + "_ssid";
        std::string pass_key = "wifi_" + std::to_string(i) + "_pass";

        // Since NVS slots are written linearly, if a slot doesn't exist, all subsequent slots are empty!
        if (!params->nvsManager->exists(ssid_key)) {
            break; 
        }

        if (params->nvsManager->exists(ssid_key) && params->nvsManager->exists(pass_key)) {
            std::string ssid = params->nvsManager->readFile(ssid_key);
            std::string pass = params->nvsManager->readFile(pass_key);

            if (tryConnectAndStabilize(params->wifiClient, ssid, pass, "NVS Slot " + std::to_string(i))) {
                ESP_LOGI(TAG, "Successfully connected using NVS slot %d!", i);
                return true;
            }
        }
    }
    return false;
}

// [CLEANED] Main background monitor task acting as the clean orchestrator
inline void wifi_background_monitor_task(void* pvParameters) {
    auto* params = static_cast<WifiMonitorParams*>(pvParameters);
    bool provisioning_active = false;

    while (true) {
        // Step 1: Check if Wi-Fi is connected
        if (params->wifi_state_->is_connected()) {
            if (provisioning_active) {
                params->wifiProvision->stop();
                provisioning_active = false;
                ESP_LOGI(TAG, "Wi-Fi is connected. Provisioning portal stopped.");
            }

            // Sleep 5 seconds before checking again
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Step 2: If NOT connected, ensure provisioning is kicked off
        if (!provisioning_active) {
            ESP_LOGW(TAG, "Wi-Fi disconnected! Starting provisioning portal...");
            params->wifiProvision->start();
            provisioning_active = true;
        }

        bool connected_this_cycle = false;

        // Step 3A: Check provisioning input first (once per cycle)
        if (handleProvisioning(params, provisioning_active)) {
            connected_this_cycle = true;
        } 
        // Step 3B: If no provisioning success, check recorded NVS slots
        else if (handleNvsSlots(params)) {
            connected_this_cycle = true;
        }

        // Step 4: If all failed in this cycle, sleep 2 seconds before retrying
        if (!connected_this_cycle) {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}





}












#include <string>
#include <memory>
#include <cstring>
#include "esp_wifi_types_generic.h"
#include "wifi_state.hpp"      // Your thread-safe state class
#include "wifi_init.hpp"
#include "wifi_provision.hpp"
#include "nvs_manager.hpp"     // Assuming your NVS manager header
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace wifi::try_connect {

static const char* TAG = "AppLoop";
static constexpr int MaxWifiNum = 33; // Define maximum stored Wi-Fi slots limit

// Structure to pass required modules into the background monitor task safely
struct WifiMonitorParams {
    std::shared_ptr<wifi_state> wifi_state_;
    std::shared_ptr<NvsManager> nvsManager;
    std::shared_ptr<WifiProvision> wifiProvision;
};

// Helper function to check if credentials already exist in NVS to avoid duplicates
inline bool isCredentialStored(std::shared_ptr<NvsManager> nvs, const std::string& ssid) {
    for (int i = 0; i < MaxWifiNum; ++i) {
        std::string slot_ssid_key = "wifi_" + std::to_string(i) + "_ssid";
        if (!nvs->exists(slot_ssid_key)) {
            break;
        }
        if (nvs->readFile(slot_ssid_key) == ssid) {
            return true;
        }
    }
    return false;
}

// Helper function to save new credentials to the next available NVS slot
inline void saveCredentialToNvs(std::shared_ptr<NvsManager> nvs, const std::string& ssid, const std::string& password) {
    if (isCredentialStored(nvs, ssid)) {
        ESP_LOGI(TAG, "Credential for SSID '%s' already exists in NVS. Skipping duplicate save.", ssid.c_str());
        return;
    }

    int target_slot = 0;
    for (int k = 0; k < MaxWifiNum; ++k) {
        std::string slot_key = "wifi_" + std::to_string(k) + "_ssid";
        if (!nvs->exists(slot_key)) {
            target_slot = k;
            break;
        }
        if (k == MaxWifiNum - 1) {
            target_slot = MaxWifiNum - 1; // Fallback to last slot if full
        }
    }

    nvs->writeFile("wifi_" + std::to_string(target_slot) + "_ssid", ssid);
    nvs->writeFile("wifi_" + std::to_string(target_slot) + "_pass", password);
    ESP_LOGI(TAG, "Successfully saved new Wi-Fi credentials to NVS slot %d", target_slot);
}

// Helper function to handle connection attempt + mode switch
inline void tryConnectAndStabilize(const std::string& ssid, const std::string& password) {
    wifi_config_t wifi_config = {};
    std::strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
    std::strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password));
    
    // Reset internal retry/state before trying a new network
    //!
    wifi_state::getInstance().reset_state();
    wifi::switch_wifi_mode(wifi_mode_t::WIFI_MODE_STA, &wifi_config);
}

// Handle user provisioning input
inline bool handleProvisioning(WifiMonitorParams* params, bool& provisioning_active) {
    if (!params->wifiProvision->hasCredentials()) {
        return false;
    }

    std::string new_ssid = params->wifiProvision->getSsid();
    std::string new_pass = params->wifiProvision->getPassword();
    
    // Clear credentials immediately so it doesn't loop endlessly on bad input
    params->wifiProvision->clearCredentials();
    params->wifiProvision->stop();
    provisioning_active = false;

    ESP_LOGI(TAG, "Attempting connection with provisioned SSID: %s", new_ssid.c_str());
    tryConnectAndStabilize(new_ssid, new_pass);

    // Give it a brief window to connect and verify state
    int timeout_seconds = 15;
    while (timeout_seconds > 0) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        if (params->wifi_state_->is_connected()) {
            ESP_LOGI(TAG, "Provisioned connection successful!");
            // Automatically save to NVS once connected!
            saveCredentialToNvs(params->nvsManager, new_ssid, new_pass);
            return true;
        }
        
        // If it hit a fatal error (wrong password, etc.), break early
        if (params->wifi_state_->has_failed()) {
            ESP_LOGE(TAG, "Provisioned connection failed with error state.");
            break;
        }
        timeout_seconds--;
    }

    return false;
}

// Handle linear NVS slot scanning
inline bool handleNvsSlots(WifiMonitorParams* params) {
    for (int i = 0; i < MaxWifiNum; ++i) {
        if (params->wifi_state_->is_connected()) {
            return true;
        }

        std::string ssid_key = "wifi_" + std::to_string(i) + "_ssid";
        std::string pass_key = "wifi_" + std::to_string(i) + "_pass";

        // Since NVS slots are written linearly, if a slot doesn't exist, stop scanning
        if (!params->nvsManager->exists(ssid_key)) {
            break;
        }

        if (params->nvsManager->exists(ssid_key) && params->nvsManager->exists(pass_key)) {
            std::string ssid = params->nvsManager->readFile(ssid_key);
            std::string pass = params->nvsManager->readFile(pass_key);

            ESP_LOGI(TAG, "Trying NVS Slot %d (SSID: %s)", i, ssid.c_str());
            tryConnectAndStabilize(ssid, pass);

            // Wait for stabilization
            int timeout_seconds = 10;
            while (timeout_seconds > 0) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (params->wifi_state_->is_connected()) {
                    ESP_LOGI(TAG, "Successfully connected using NVS slot %d!", i);
                    return true;
                }
                if (params->wifi_state_->has_failed()) {
                    break; // Move to next slot or fallback
                }
                timeout_seconds--;
            }
        }
    }
    return false;
}

// Main background monitor task acting as the clean orchestrator
inline void wifi_background_monitor_task(void* pvParameters) {
    auto* params = static_cast<WifiMonitorParams*>(pvParameters);
    bool provisioning_active = false;

    while (true) {
        // Step 1: Check if Wi-Fi is connected
        if (params->wifi_state_->is_connected()) {
            if (provisioning_active) {
                params->wifiProvision->stop();
                provisioning_active = false;
                ESP_LOGI(TAG, "Wi-Fi is connected. Provisioning portal stopped.");
            }

            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Step 2: If NOT connected, ensure provisioning portal is active
        if (!provisioning_active) {
            ESP_LOGW(TAG, "Wi-Fi disconnected! Starting provisioning portal...");
            params->wifiProvision->start();
            provisioning_active = true;
        }

        bool connected_this_cycle = false;

        // Step 3A: Check provisioning input first
        if (handleProvisioning(params, provisioning_active)) {
            connected_this_cycle = true;
        } 
        // Step 3B: If no provisioning input, scan recorded NVS slots
        else if (handleNvsSlots(params)) {
            connected_this_cycle = true;
        }

        // Step 4: If all attempts failed in this cycle, sleep before retrying
        if (!connected_this_cycle) {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

} // namespace wifi::try_connect




























