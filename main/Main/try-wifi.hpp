#pragma once






 #include<string>
#include "wifi_client.hpp"
#include "wifi_provision.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



namespace wifi::try_connect {



static const char* TAG = "AppLoop";
static constexpr int MaxWifiNum = 33; // Define maximum stored Wi-Fi slots limit


// Structure to pass required modules into the background monitor task safely
struct WifiMonitorParams {
    std::shared_ptr<WifiClient> wifiClient;
    std::shared_ptr<NvsManager> nvsManager;
    std::shared_ptr<WifiProvision> wifiProvision;
};




// [MODULAR] Helper function to handle connection attempt + stabilization polling + fast-fail
inline bool tryConnectAndStabilize( std::shared_ptr<WifiClient> wifiClient, const std::string& ssid, const std::string& pass, const std::string& context_name) {
    ESP_LOGI(TAG, "Testing %s credentials for SSID: [%s]", context_name.c_str(), ssid.c_str());
    
    if (!wifiClient->connect(ssid, pass)) {
        return false;
    }

    // Give the asynchronous connection handshake time to stabilize and obtain an IP
    for (int attempt = 0; attempt < 20; ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(500)); // Check connection state every 500ms (up to 10 seconds)
        
        // Check raw disconnect telemetry for fast-fail (e.g. wrong password or out of range)
        int reason = wifiClient->getDisconnectReason();
        if (reason == WIFI_REASON_AUTH_FAIL) {
            ESP_LOGE(TAG, "Fast-fail [%s]: Incorrect Wi-Fi password (Reason 205).", context_name.c_str());
            return false;
        }
        if (reason == WIFI_REASON_NO_AP_FOUND) {
            ESP_LOGE(TAG, "Fast-fail [%s]: SSID not found or out of range (Reason 201).", context_name.c_str());
            return false;
        }

        if (wifiClient->isConnected()) {
            return true;
        }
    }

    ESP_LOGW(TAG, "Connection attempt timed out for [%s].", ssid.c_str());
    return false;
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

    if (tryConnectAndStabilize(params->wifiClient, new_ssid, new_pass, "Provisioned")) {
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
        if (params->wifiClient->isConnected()) {
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