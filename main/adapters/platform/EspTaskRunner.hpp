#pragma once

// Adapter: core::ITaskRunner on top of FreeRTOS tasks.

#include "core/ports.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace adapters {

class EspTaskRunner : public core::ITaskRunner {
public:
    TaskId start(const char* name, uint32_t stack_bytes, int priority, int core,
                 void (*fn)(void*), void* arg) override {
        TaskHandle_t handle = nullptr;
        if (xTaskCreatePinnedToCore(fn, name, stack_bytes, arg, priority, &handle, core) != pdPASS) {
            return nullptr;
        }
        return static_cast<TaskId>(handle);
    }

    void stop(TaskId task) override {
        if (task != nullptr) {
            vTaskDelete(static_cast<TaskHandle_t>(task));
        }
    }

    void delayMs(uint32_t ms) override { vTaskDelay(pdMS_TO_TICKS(ms)); }

    void exitCurrentTask() override { vTaskDelete(nullptr); }
};

} // namespace adapters
