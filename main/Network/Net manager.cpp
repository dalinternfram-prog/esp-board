// Network module: worker task + delegation to the AppContext session.
// Protocol logic lives in core::SessionManager; adapters live in AppContext.

#include "Net manager.hpp"

#include <atomic>
#include <memory>
#include <vector>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_context.hpp"

namespace Network {

static const char* TAG = "NETWORK_MANAGER";

namespace {

constexpr uint32_t kWorkerTickMs     = 10;
constexpr uint32_t kWorkerStackBytes = 8192;
constexpr int      kWorkerPriority   = 5;
constexpr int      kWorkerCore       = 1;

struct Worker {
    TaskHandle_t      handle = nullptr;
    std::atomic<bool> running{false};
};

} // namespace

static std::unique_ptr<Worker> s_worker;

static void network_worker_task(void* arg) {
    auto* w = static_cast<Worker*>(arg);
    ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();
    while (w->running.load()) {
        if (app::context().session) {
            app::context().session->tick();
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(kWorkerTickMs));
    }

    w->handle = nullptr;
    vTaskDelete(nullptr);
}

void init(const char* host, const char* port) {
    if (s_worker && s_worker->running.load()) {
        ESP_LOGW(TAG, "Network already initialized");
        return;
    }

    app::context().buildSession(host, port);

    if (!s_worker) {
        s_worker = std::make_unique<Worker>();
    }
    s_worker->running.store(true);

    BaseType_t res = xTaskCreatePinnedToCore(
        network_worker_task, "net_worker_io", kWorkerStackBytes,
        s_worker.get(), kWorkerPriority, &s_worker->handle, kWorkerCore);

    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create network worker task");
        s_worker->running.store(false);
    }

    ESP_LOGI(TAG, "Network manager initialized");
}

void close_connection() {
    if (!s_worker) {
        return;
    }

    s_worker->running.store(false);
    for (int i = 0; i < 100 && s_worker->handle != nullptr; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    s_worker.reset();

    if (app::context().session) {
        app::context().session->stop();
    }
    ESP_LOGI(TAG, "Network connection closed");
}

int send(const std::vector<uint8_t>& data) {
    auto& session = app::context().session;
    return session ? session->send(data) : -1;
}

std::vector<uint8_t> poll_and_receive() {
    auto& session = app::context().session;
    if (!session) {
        return {};
    }
    auto msg = session->poll();
    return msg.has_value() ? std::move(*msg) : std::vector<uint8_t>{};
}

} // namespace Network
