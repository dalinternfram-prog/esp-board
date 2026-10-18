#pragma once






 #include<string>
#include "wifi_client.hpp"
#include "wifi_provision.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



namespace wifi::try_connect {



static const char* TAG = "AppLoop";
static constexpr int MaxWifiNum = 333; // Define maximum stored Wi-Fi slots limit


// Structure to pass required modules into the background monitor task safely
struct WifiMonitorParams {
    std::shared_ptr<WifiClient> wifiClient;
    std::shared_ptr<NvsManager> nvsManager;
    std::shared_ptr<WifiProvision> wifiProvision;
};

 // Background infinite loop task following your exact interleaved state logic
inline void wifi_background_monitor_task(void* pvParameters) {
    auto* params = static_cast<WifiMonitorParams*>(pvParameters);
    bool provisioning_active = false;

    while (true) {
        // Step 1: Check if Wi-Fi is connected
        if (params->wifiClient->isConnected()) {
            // If connected, make sure provisioning portal is turned off
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

        // Step 3: Interleaved check (Check provisioning input + step through NVS slots)
        for (int i = 0; i < MaxWifiNum; ++i) {
            // Re-check connection status on the fly
            if (params->wifiClient->isConnected()) {
                connected_this_cycle = true;
                break;
            }

            // A. Ask/Check if provisioning input is available
            if (params->wifiProvision->hasCredentials()) {
                std::string new_ssid = params->wifiProvision->getSsid();
                std::string new_pass = params->wifiProvision->getPassword();

                ESP_LOGI(TAG, "Testing provisioned credentials for SSID: [%s]", new_ssid.c_str());
                if (params->wifiClient->connect(new_ssid, new_pass)) {
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
                    connected_this_cycle = true;
                    break;
                }
            }

            // B. Check recorded NVS slot (One check)
            std::string ssid_key = "wifi_" + std::to_string(i) + "_ssid";
            std::string pass_key = "wifi_" + std::to_string(i) + "_pass";

            if (params->nvsManager->exists(ssid_key) && params->nvsManager->exists(pass_key)) {
                std::string ssid = params->nvsManager->readFile(ssid_key);
                std::string pass = params->nvsManager->readFile(pass_key);

                ESP_LOGI(TAG, "Checking NVS slot %d (SSID: [%s])...", i, ssid.c_str());
                if (params->wifiClient->connect(ssid, pass)) {
                    ESP_LOGI(TAG, "Successfully connected using NVS slot %d!", i);
                    connected_this_cycle = true;
                    break;
                }
            }
        }

        // Step 4: If all failed in this cycle, sleep 2 seconds, check and ask again
        if (!connected_this_cycle) {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

}