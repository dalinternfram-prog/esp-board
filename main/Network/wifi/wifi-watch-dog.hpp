  
#pragma once

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

namespace wifi {

 class watchDog{

private:
std::string stored_ssid;
std::string stored_pass;

public:


    void init();


    void try_connect(std::string ssid,std::string pass);


 };

} // namespace wifi::try_connect




























