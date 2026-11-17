#include "button_driver.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "tca9555_driver.h"

#include "general/input/button_state_machine.hpp"

static const char *TAG = "button_driver";

// One pure state machine per key; this driver only reads GPIO and dispatches.
static general::input::ButtonStateMachine s_keys[3];
static key_callback_t s_callback = NULL;
static TimerHandle_t s_timer = NULL;
static void *s_user_data = NULL;

// 从掩码获取按键状态（按下返回true，根据硬件修改）
static bool get_key_state(uint32_t mask, key_id_t id) {
    // 假设：按键按下时对应位为0（低电平），未按下为1（高电平）
    return !(mask & (1ULL << id));
}

// 100ms扫描一次
static void scan_timer_ccan(void *arg) {
    (void)arg;
    while (1) {
        uint32_t mask;
        // 一次读取所有关注的引脚（仅一次I2C操作）
        esp_err_t ret = esp_io_expander_get_level(
            io_expander,
            (1ULL << KEY_ID_9) | (1ULL << KEY_ID_10) | (1ULL << KEY_ID_11),
            &mask);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "读取IO扩展芯片失败");
            return;
        }

        const uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000);

        for (int i = 0; i < 3; i++) {
            const key_id_t key_id = static_cast<key_id_t>(KEY_ID_9 + i);
            const bool pressed = get_key_state(mask, key_id);
            const general::input::ButtonEvent event = s_keys[i].update(pressed, now);

            if (event == general::input::ButtonEvent::ShortPress && s_callback) {
                s_callback(key_id, KEY_EVENT_SHORT_PRESS, s_user_data);
            } else if (event == general::input::ButtonEvent::LongPress && s_callback) {
                s_callback(key_id, KEY_EVENT_LONG_PRESS, s_user_data);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 初始化
esp_err_t key_module_init(void *user_data) {
    s_user_data = user_data;
    xTaskCreatePinnedToCore(scan_timer_ccan, "key_scan", 4096, NULL, 3, NULL, 0);
    return ESP_OK;
}

// 注册回调
esp_err_t key_register_callback(key_callback_t callback) {
    if (!callback) return ESP_ERR_INVALID_ARG;
    s_callback = callback;
    return ESP_OK;
}

// 反初始化
esp_err_t key_module_deinit(void) {
    if (s_timer) {
        xTimerStop(s_timer, 0);
        xTimerDelete(s_timer, 0);
    }
    s_callback = NULL;
    return ESP_OK;
}
