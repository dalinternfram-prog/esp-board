#include "hardware.hpp"

#include "esp_err.h"
#include "esp_log.h"

#include "bsp_board.h"
#include "rgb_led_driver.h"
#include "tca9555_driver.h"

static const char* TAG = "hardware";

namespace hardware {

void boot(const core::AudioConfig& config) {
    ESP_LOGI(TAG, "Booting hardware...");

    ESP_ERROR_CHECK(esp_board_init(config.sample_rate,
                                   static_cast<int>(config.channel_count),
                                   static_cast<int>(config.bits_per_chan)));
    tca9555_driver_init();
    esp_sdcard_init(const_cast<char*>("/sdcard"), 10);

    // Must run before any set_rgb_mode/set_rgb_color call: it creates the
    // message queue and the LED task used by EspIndicator.
    RGB_Example();

    ESP_LOGI(TAG, "Hardware ready");
}

} // namespace hardware
