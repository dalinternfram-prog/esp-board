#include "hardware.hpp"

#include "esp_err.h"
#include "esp_log.h"

#include "app_context.hpp"
#include "bsp_board.h"
#include "core/app/arguments.hpp"
#include "tca9555_driver.h"

static const char *TAG = "hardware";

namespace hardware {

void boot() {
    ESP_LOGI(TAG, "Booting hardware...");

    ESP_ERROR_CHECK(esp_board_init(macroPhone::argument::sample_rate,
                                   macroPhone::argument::channel_count,
                                   macroPhone::argument::bits_per_chan));
    tca9555_driver_init();
    esp_sdcard_init(const_cast<char *>("/sdcard"), 10);

    // Media/UI behaviour lives in core/app/MediaController; AppContext wires the
    // ESP adapters (player, indicator, buttons, speech, playlist).
    app::context().startMedia();

    ESP_LOGI(TAG, "Hardware ready");
}

} // namespace hardware
