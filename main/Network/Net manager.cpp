// Network module: thin composition root for the core SessionManager.
//
// All protocol logic (handshake, liveness, reconnect, encryption, queues) now
// lives in main/core/session/SessionManager.* and is platform-agnostic. This
// file only wires concrete ESP adapters to the core ports.

#include "Net manager.hpp"

#include <atomic>
#include <memory>
#include <vector>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "adapters/crypto/EspCrypto.hpp"
#include "adapters/link/EspWifiLink.hpp"
#include "adapters/platform/EspPlatform.hpp"
#include "adapters/transport/KcpUdpTransport.hpp"
#include "core/session/SessionManager.hpp"

namespace Network {

static const char* TAG = "NETWORK_MANAGER";

namespace {

constexpr uint32_t kWorkerTickMs     = 10;
constexpr uint32_t kWorkerStackBytes = 8192;
constexpr int      kWorkerPriority   = 5;
constexpr int      kWorkerCore       = 1;

// Composition root: the only place that knows the concrete adapters.
struct NetContext {
    adapters::EspClock    clock;
    adapters::EspRandom   random;
    adapters::EspLogger   logger;
    adapters::EspCrypto   crypto;
    adapters::EspWifiLink link;

    std::unique_ptr<adapters::KcpUdpTransport> transport;
    std::unique_ptr<core::SessionManager>      session;

    TaskHandle_t      worker_handle = nullptr;
    std::atomic<bool> worker_running{false};
};

} // namespace

static std::unique_ptr<NetContext> s_ctx;

static void network_worker_task(void* arg) {
    auto* ctx = static_cast<NetContext*>(arg);
    ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();
    while (ctx->worker_running.load()) {
        ctx->session->tick();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(kWorkerTickMs));
    }

    ctx->worker_handle = nullptr;
    vTaskDelete(nullptr);
}

void init(const char* host, const char* port) {
    if (s_ctx) {
        ESP_LOGW(TAG, "Network already initialized");
        return;
    }

    s_ctx = std::make_unique<NetContext>();
    const uint32_t conv_id = s_ctx->random.u32();

    s_ctx->transport = std::make_unique<adapters::KcpUdpTransport>(host, port, conv_id);
    s_ctx->session = std::make_unique<core::SessionManager>(
        *s_ctx->transport, s_ctx->link, s_ctx->clock,
        s_ctx->random, s_ctx->crypto, s_ctx->logger);
    s_ctx->session->start();

    s_ctx->worker_running.store(true);
    BaseType_t res = xTaskCreatePinnedToCore(
        network_worker_task,
        "net_worker_io",
        kWorkerStackBytes,
        s_ctx.get(),
        kWorkerPriority,
        &s_ctx->worker_handle,
        kWorkerCore);

    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create network worker task");
        s_ctx->worker_running.store(false);
    }

    ESP_LOGI(TAG, "Network manager initialized (conv_id: %u)", static_cast<unsigned>(conv_id));
}

void close_connection() {
    if (!s_ctx) {
        return;
    }

    // Ask the worker to stop and give it a moment to leave tick().
    s_ctx->worker_running.store(false);
    for (int i = 0; i < 100 && s_ctx->worker_handle != nullptr; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (s_ctx->session) {
        s_ctx->session->stop();
    }
    s_ctx.reset();
    ESP_LOGI(TAG, "Network connection closed");
}

int send(const std::vector<uint8_t>& data) {
    if (!s_ctx || !s_ctx->session) {
        return -1;
    }
    return s_ctx->session->send(data);
}

std::vector<uint8_t> poll_and_receive() {
    if (!s_ctx || !s_ctx->session) {
        return {};
    }
    auto msg = s_ctx->session->poll();
    return msg.has_value() ? std::move(*msg) : std::vector<uint8_t>{};
}

} // namespace Network
