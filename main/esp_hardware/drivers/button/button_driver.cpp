#include "button_driver.h"

#include <atomic>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "tca9555_driver.h"

#include "general/input/button_state_machine.hpp"

static const char *TAG = "button_driver";

// One pure state machine per key; this driver only reads GPIO and dispatches.
static general::input::ButtonStateMachine s_keys[3];
static key_callback_t s_callback = NULL;
static std::atomic<bool> s_running{false};
static std::atomic<bool> s_task_alive{false};
static void *s_user_data = NULL;

// 从掩码获取按键状态（按下返回true，根据硬件修改）
static bool get_key_state(uint32_t mask, key_id_t id) {
    // 假设：按键按下时对应位为0（低电平），未按下为1（高电平）
    return !(mask & (1ULL << id));
}

// 100ms扫描一次
static void scan_timer_ccan(void *arg) {
    (void)arg;
    while (s_running.load()) {
        uint32_t mask;
        // 一次读取所有关注的引脚（仅一次I2C操作）
        esp_err_t ret = esp_io_expander_get_level(
            io_expander,
            (1ULL << KEY_ID_9) | (1ULL << KEY_ID_10) | (1ULL << KEY_ID_11),
            &mask);
        if (ret != ESP_OK) {
            // Never return from a FreeRTOS task; keep scanning on transient
            // I2C failures (returning would abort the task).
            ESP_LOGE(TAG, "读取IO扩展芯片失败");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
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

    // Clean self-exit: drop the liveness flag first so key_module_deinit()
    // can wait on it; then self-delete.
    s_task_alive.store(false);
    vTaskDelete(NULL);
}

// 初始化
esp_err_t key_module_init(void *user_data) {
    if (s_task_alive.load()) {
        return ESP_ERR_INVALID_STATE;   // already running
    }
    s_user_data = user_data;
    s_running.store(true);
    s_task_alive.store(true);           // set before create: the task can exit instantly
    BaseType_t res = xTaskCreatePinnedToCore(scan_timer_ccan, "key_scan", 4096,
                                             NULL, 3, NULL, 0);
    if (res != pdPASS) {
        s_task_alive.store(false);
        s_running.store(false);
        return ESP_FAIL;
    }
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
    // Ask the task to leave its loop; it exits at the next 100 ms scan boundary,
    // clears its own handle and self-deletes. Wait on the handle (not a fixed
    // delay) and never kill it from outside while it may be mid-I2C.
    s_running.store(false);
    for (int i = 0; i < 50 && s_task_alive.load(); ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    s_callback = NULL;
    return ESP_OK;
}
