#include "protocol/client/audio/audio_protocol.hpp"

#include <memory>
#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "audio_capture_driver.hpp"
#include "macroPhone/arguments.hpp"
#include "Opus.hpp"
#include "Network/Net manager.hpp" // For Network::send

namespace AudioService {

    static const char* TAG = "AUDIO_SERVICE";

    // Task handle & lifecycle control
    static TaskHandle_t         s_audio_task_handle = nullptr;
    static std::atomic<bool>    s_should_run{false};

    static void audio_processing_task(void* pvParameters) {
        ESP_LOGI(TAG, "Audio task created & initializing on Core %d", xPortGetCoreID());

        // [CHANGE]: Hardware driver and Opus are instantiated locally on the stack.
        // When the loop ends upon stop(), their destructors will automatically free
        // I2S DMA buffers and Opus memory cleanly.
        auto audioCaptureDriver = std::make_shared<Hardware::AudioCaptureDriver>();
        auto opus = std::make_shared<Opus>();

        opus->init(
            macroPhone::argument::sample_rate,
            macroPhone::argument::channel_count,
            macroPhone::argument::bits_per_chan,
            macroPhone::argument::target_frame_duration_ms
        );

        uint32_t frameCount = 0;

        // Loop runs until stop() sets s_should_run to false
        while (s_should_run.load()) {
            // 2. Read new PCM frame from hardware
            auto ret = audioCaptureDriver->readSamples();
            if (ret == ESP_OK) {
                // 3. Get modern C++ std::span pointing to the raw PSRAM data
                auto raw_pcm = audioCaptureDriver->getFrameBuffer(); 
                auto compressed_pcm = opus->encode(raw_pcm.data()); 

    // 1. Capture timestamp in milliseconds
    uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

    // 2. Encode into audio protocol packet [0xA3 | timestamp | length | Opus data]
    std::vector<uint8_t> packet = Protocol::Audio::encode_audio(compressed_pcm, now_ms);
    Network::send(packet); 
                frameCount++;
                  // ESP_LOGW(TAG, "frameCount = %u",frameCount);
            } else {
                ESP_LOGW(TAG, "Failed to read audio samples, retrying...");
            }

            vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep Task Watchdog happy
        }

        ESP_LOGI(TAG, "Exiting audio loop: cleaning up drivers and deleting task");

        // Explicitly clear pointer and self-delete to release task stack
        s_audio_task_handle = nullptr;
        vTaskDelete(NULL);
    }

    void start() {
        if (s_audio_task_handle != nullptr) {
            ESP_LOGW(TAG, "Audio task is already running, ignoring start request");
            return;
        }

        s_should_run.store(true);

        // [CHANGE]: Create the task fresh on CPU Core 1
        BaseType_t res = xTaskCreatePinnedToCore(
            audio_processing_task,
            "audio_proc_task",
            39152,                  //  
            nullptr,
            5,                      // High priority
            &s_audio_task_handle,
            1                       // Pinned to CPU Core 1
        );

        if (res == pdPASS) {
            ESP_LOGI(TAG, "Audio task successfully initialized and started");
        } else {
            ESP_LOGE(TAG, "Failed to create audio task!");
            s_should_run.store(false);
        }
    }

    void stop() {
        if (s_audio_task_handle == nullptr) {
            ESP_LOGW(TAG, "Audio task already stopped or not initialized");
            return;
        }

        ESP_LOGI(TAG, "Stopping audio task...");
        // Signal task loop to break
        s_should_run.store(false);

        // Wait briefly (up to 200ms) for the task to exit cleanly and delete itself
        int timeout_ms = 200;
        while (s_audio_task_handle != nullptr && timeout_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            timeout_ms -= 10;
        }

        // Fallback: If task hung inside a blocking driver read, force delete it
        if (s_audio_task_handle != nullptr) {
            ESP_LOGW(TAG, "Force deleting audio task after timeout");
            TaskHandle_t temp_handle = s_audio_task_handle;
            s_audio_task_handle = nullptr;
            vTaskDelete(temp_handle);
        }

        ESP_LOGI(TAG, "Audio task completely stopped and deleted");
    }

    bool is_running() {
        return (s_audio_task_handle != nullptr);
    }

} // namespace AudioService