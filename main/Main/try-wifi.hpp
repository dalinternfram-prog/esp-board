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
                    
                    // [NEW] Give the asynchronous connection handshake time to stabilize and obtain an IP before declaring success
                    bool stable_connection = false;
                    for (int attempt = 0; attempt < 20; ++attempt) {
                        vTaskDelay(pdMS_TO_TICKS(500)); // Check connection state every 500ms (up to 10 seconds)
                        if (params->wifiClient->isConnected()) {
                            stable_connection = true;
                            break;
                        }
                    }

                    if (stable_connection) {
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
                    } else {
                        ESP_LOGW(TAG, "Connection attempt timed out. Router took too long or credentials failed.");
                    }
                }
            }

            // B. Check recorded NVS slot (One check)
            std::string ssid_key = "wifi_" + std::to_string(i) + "_ssid";
            std::string pass_key = "wifi_" + std::to_string(i) + "_pass";

            // [NEW] Since NVS slots are written linearly, if a slot doesn't exist, all subsequent slots are guaranteed to be empty too!
            if (!params->nvsManager->exists(ssid_key)) {
                break; 
            }

            if (params->nvsManager->exists(ssid_key) && params->nvsManager->exists(pass_key)) {
                std::string ssid = params->nvsManager->readFile(ssid_key);
                std::string pass = params->nvsManager->readFile(pass_key);

                ESP_LOGI(TAG, "Checking NVS slot %d (SSID: [%s])...", i, ssid.c_str());
                if (params->wifiClient->connect(ssid, pass)) {
                    
                    // [NEW] Give saved NVS slot connections a stable handshake timeout window as well
                    bool stable_nvs_connection = false;
                    for (int attempt = 0; attempt < 20; ++attempt) {
                        vTaskDelay(pdMS_TO_TICKS(500));
                        if (params->wifiClient->isConnected()) {
                            stable_nvs_connection = true;
                            break;
                        }
                    }

                    if (stable_nvs_connection) {
                        ESP_LOGI(TAG, "Successfully connected using NVS slot %d!", i);
                        connected_this_cycle = true;
                        break;
                    } else {
                        ESP_LOGW(TAG, "NVS slot %d connection timed out.", i);
                    }
                }
            }
        }

        // Step 4: If all failed in this cycle, sleep 2 seconds, check and ask again
        if (!connected_this_cycle) {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}









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
                    
                    // Give the asynchronous connection handshake time to stabilize and obtain an IP before declaring success
                    bool stable_connection = false;
                    for (int attempt = 0; attempt < 20; ++attempt) {
                        vTaskDelay(pdMS_TO_TICKS(500)); // Check connection state every 500ms (up to 10 seconds)
                        
                        // [NEW] Check raw disconnect telemetry for fast-fail (e.g. wrong password or out of range)
                        int reason = params->wifiClient->getDisconnectReason();
                        if (reason == WIFI_REASON_AUTH_FAIL) {
                            ESP_LOGE(TAG, "Fast-fail: Incorrect Wi-Fi password (Reason 205). Aborting wait.");
                            break;
                        }
                        if (reason == WIFI_REASON_NO_AP_FOUND) {
                            ESP_LOGE(TAG, "Fast-fail: SSID not found or out of range (Reason 201). Aborting wait.");
                            break;
                        }

                        if (params->wifiClient->isConnected()) {
                            stable_connection = true;
                            break;
                        }
                    }

                    if (stable_connection) {
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
                    } else {
                        ESP_LOGW(TAG, "Connection attempt timed out or failed fast.");
                    }
                }
            }

            // B. Check recorded NVS slot (One check)
            std::string ssid_key = "wifi_" + std::to_string(i) + "_ssid";
            std::string pass_key = "wifi_" + std::to_string(i) + "_pass";

            // Since NVS slots are written linearly, if a slot doesn't exist, all subsequent slots are guaranteed to be empty too!
            if (!params->nvsManager->exists(ssid_key)) {
                break; 
            }

            if (params->nvsManager->exists(ssid_key) && params->nvsManager->exists(pass_key)) {
                std::string ssid = params->nvsManager->readFile(ssid_key);
                std::string pass = params->nvsManager->readFile(pass_key);

                ESP_LOGI(TAG, "Checking NVS slot %d (SSID: [%s])...", i, ssid.c_str());
                if (params->wifiClient->connect(ssid, pass)) {
                    
                    // Give saved NVS slot connections a stable handshake timeout window as well
                    bool stable_nvs_connection = false;
                    for (int attempt = 0; attempt < 20; ++attempt) {
                        vTaskDelay(pdMS_TO_TICKS(500));
                        
                        // [NEW] Fast-fail check for NVS slot attempts as well
                        int reason = params->wifiClient->getDisconnectReason();
                        if (reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_NO_AP_FOUND) {
                            ESP_LOGW(TAG, "NVS slot %d failed fast with reason code %d.", i, reason);
                            break;
                        }

                        if (params->wifiClient->isConnected()) {
                            stable_nvs_connection = true;
                            break;
                        }
                    }

                    if (stable_nvs_connection) {
                        ESP_LOGI(TAG, "Successfully connected using NVS slot %d!", i);
                        connected_this_cycle = true;
                        break;
                    } else {
                        ESP_LOGW(TAG, "NVS slot %d connection timed out.", i);
                    }
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