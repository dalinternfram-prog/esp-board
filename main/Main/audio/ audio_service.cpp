// AudioService: application-layer composition for capture -> encode -> send.
// Hardware (I2S) and codec (Opus) sit behind core ports; AudioPipeline is
// platform-agnostic. This file only wires the concrete adapters together.

#include <atomic>
#include <cstdint>
#include <memory>

#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "Network/Net manager.hpp"

#include "adapters/audio/EspI2sAudioSource.hpp"
#include "adapters/audio/OpusEncoder.hpp"
#include "adapters/platform/EspPlatform.hpp"
#include "audio_protocol.hpp"      // protocol::IoT::client::encodeAudio
#include "core/audio/AudioPipeline.hpp"
#include "turn_on_audio.hpp"       // protocol::IoT::client::encodeRecordAck

namespace AudioService {

static const char* TAG = "AUDIO_SERVICE";

namespace {

constexpr uint32_t kLoopDelayMs    = 10;
constexpr uint32_t kTaskStackBytes = 39152;
constexpr int      kTaskPriority   = 5;
constexpr int      kTaskCore       = 1;

struct AudioContext {
    TaskHandle_t      task_handle = nullptr;
    std::atomic<bool> running{false};
};

} // namespace

static std::unique_ptr<AudioContext> s_ctx;

static int64_t now_ms() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<int64_t>(tv.tv_sec) * 1000LL + tv.tv_usec / 1000LL;
}

static void audio_processing_task(void* /*arg*/) {
    ESP_LOGI(TAG, "Audio task created & initializing on Core %d", xPortGetCoreID());

    adapters::EspI2sAudioSource source;
    adapters::OpusEncoder       encoder;
    adapters::EspClock          clock;
    adapters::EspLogger         logger;

    if (!source.open() || !encoder.init()) {
        ESP_LOGE(TAG, "Audio pipeline init failed");
        s_ctx->task_handle = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    core::AudioPipeline pipeline(
        source, encoder, clock, logger,
        [](const core::Bytes& compressed, uint32_t timestamp_ms) {
            auto packet = protocol::IoT::client::encodeAudio(compressed, timestamp_ms);
            Network::send(packet);
        });

    while (s_ctx->running.load()) {
        pipeline.step();
        vTaskDelay(pdMS_TO_TICKS(kLoopDelayMs));
    }

    source.close();
    ESP_LOGI(TAG, "Exiting audio loop: releasing drivers");
    s_ctx->task_handle = nullptr;
    vTaskDelete(nullptr);
}

void start() {
    if (!s_ctx) {
        s_ctx = std::make_unique<AudioContext>();
    }
    if (s_ctx->task_handle != nullptr) {
        ESP_LOGW(TAG, "Audio task is already running, ignoring start request");
        return;
    }

    s_ctx->running.store(true);
    BaseType_t res = xTaskCreatePinnedToCore(
        audio_processing_task, "audio_proc_task", kTaskStackBytes,
        nullptr, kTaskPriority, &s_ctx->task_handle, kTaskCore);

    if (res == pdPASS) {
        ESP_LOGI(TAG, "Audio task successfully initialized and started");
        auto ack = protocol::IoT::client::encodeRecordAck(
            protocol::IoT::RecordCommand::START, now_ms());
        Network::send(ack);
    } else {
        ESP_LOGE(TAG, "Failed to create audio task!");
        s_ctx->running.store(false);
    }
}

void stop() {
    if (!s_ctx || s_ctx->task_handle == nullptr) {
        ESP_LOGW(TAG, "Audio task already stopped or not initialized");
        return;
    }

    ESP_LOGI(TAG, "Stopping audio task...");
    s_ctx->running.store(false);

    int timeout_ms = 200;
    while (s_ctx->task_handle != nullptr && timeout_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout_ms -= 10;
    }

    if (s_ctx->task_handle != nullptr) {
        ESP_LOGW(TAG, "Force deleting audio task after timeout");
        TaskHandle_t temp = s_ctx->task_handle;
        s_ctx->task_handle = nullptr;
        vTaskDelete(temp);
    }

    ESP_LOGI(TAG, "Audio task completely stopped and deleted");
    auto ack = protocol::IoT::client::encodeRecordAck(
        protocol::IoT::RecordCommand::STOP, now_ms());
    Network::send(ack);
}

bool is_running() {
    return s_ctx && s_ctx->task_handle != nullptr;
}

} // namespace AudioService
