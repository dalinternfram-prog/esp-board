 

#include "Net manager.hpp"

// Internal Network Dependencies
#include "KcpClient.hpp"
#include "WiFiManager.hpp"
#include "encryption/interface.hpp"

// System & FreeRTOS Headers
#include <mutex>
#include <queue>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Network {

    static const char* TAG = "NETWORK_MANAGER";

    // =========================================================================
    // Handshake State Engine
    // =========================================================================
    enum class HandshakeState {
        IDLE,           // Offline or awaiting connection
        WAIT_ACK,       // UDP handshake packet dispatched, awaiting ECDH response
        ESTABLISHED     // Handshake complete, AES key active for live traffic
    };

    // =========================================================================
    // Static / Internal Handles
    // =========================================================================
    static std::unique_ptr<KcpClient>   s_kcp_client;
    static uint32_t                     s_conv_id = 0;

    // Crypto & Handshake session state
    static std::vector<uint8_t>         s_aes_key;
    static HandshakeState               s_hs_state = HandshakeState::IDLE;
    static std::vector<uint8_t>         s_cached_handshake_pkt; 
    static uint32_t                     s_last_handshake_attempt_ms = 0;
    static uint32_t                     s_handshake_retry_count = 0;

    // Worker Task Management
    static TaskHandle_t                 s_worker_task_handle = nullptr;
    static std::atomic<bool>            s_worker_running{false};

    // Double-Buffering (Outbound)
    static std::mutex                               s_tx_mutex;
    static std::vector<std::vector<uint8_t>>        s_incoming_queue; // Appended by producer threads
    static std::vector<std::vector<uint8_t>>        s_sending_queue;  // Consumed by worker task

    // Inbound Rx Queue (Thread-safe consumer access)
    static std::mutex                               s_rx_mutex;
    static std::queue<std::vector<uint8_t>>         s_rx_queue;

    // Tuning Parameters
    static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Initial retry backoff
    static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Upper backoff ceiling
    static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;  // Producer drop threshold (RAM backpressure)
    static constexpr TickType_t WORKER_TICK_DELAY     = pdMS_TO_TICKS(10); // Standard 10ms KCP tick

    // Forward declaration of internal task & helpers
    static void network_worker_task(void* pvParameters);
    static void handle_autonomous_handshake(uint32_t now_ms);

    // =========================================================================
    // Public API Implementation
    // =========================================================================

    void init(const char* host, const char* port) {
        s_conv_id = esp_random();

        // 1. Initialize low-level KCP Client
        s_kcp_client = std::make_unique<KcpClient>(s_conv_id);
        s_kcp_client->initialize(host, port);

        // 2. Reset session & handshake state
        s_aes_key.clear();
        s_cached_handshake_pkt.clear();
        s_hs_state = HandshakeState::IDLE;
        s_handshake_retry_count = 0;

        // 3. Clear queues
        {
            std::lock_guard<std::mutex> lock_tx(s_tx_mutex);
            s_incoming_queue.clear();
            s_sending_queue.clear();
        }
        {
            std::lock_guard<std::mutex> lock_rx(s_rx_mutex);
            std::queue<std::vector<uint8_t>> empty_q;
            std::swap(s_rx_queue, empty_q);
        }

        // 4. Spawn dedicated worker task (Pinned to Core 1)
        if (!s_worker_running.load()) {
            s_worker_running.store(true);
            xTaskCreatePinnedToCore(
                network_worker_task,
                "net_worker_io",
                8192,
                nullptr,
                5,//TODO change to 5
                &s_worker_task_handle,
                1
            );
        }

        ESP_LOGI(TAG, "Network manager initialized (conv_id: %u)", s_conv_id);
    }

    void close_connection() {
        s_worker_running.store(false);

        if (s_kcp_client) {
            s_kcp_client->close_connection();
        }

        s_hs_state = HandshakeState::IDLE;
        ESP_LOGI(TAG, "Network connection closed");
    } 
int send(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(s_tx_mutex);

    // --- Performance Tracking State ---
    static int64_t  s_last_report_us = 0;
    static uint32_t s_window_sent    = 0;
    static uint32_t s_window_dropped = 0;
    static size_t   s_window_bytes   = 0;

    int64_t now_us = esp_timer_get_time();
    if (s_last_report_us == 0) {
        s_last_report_us = now_us;
    }

    // 1. Backpressure Check
    if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
        s_window_dropped++;
        ESP_LOGW(TAG, "Outgoing RAM queue full; message rejected (Backpressure)");
        return -1;
    }

    // 2. Queue the Packet
    s_incoming_queue.push_back(data);
    s_window_sent++;
    s_window_bytes += data.size();

    // 3. Periodic 1-Second Send Frequency Report
    int64_t elapsed_us = now_us - s_last_report_us;
    if (elapsed_us >= 1000000ULL) {
        float elapsed_sec = elapsed_us / 1000000.0f;
        float pps = s_window_sent / elapsed_sec;
        float kb_per_sec = (s_window_bytes / 1024.0f) / elapsed_sec;

        ESP_LOGI(TAG, "[TX STATS 1s] Rate: %.1f pkts/s | Bandwidth: %.2f KB/s | Dropped: %u | Queue Depth: %u",
                 pps,
                 kb_per_sec,
                 s_window_dropped,
                 static_cast<unsigned>(s_incoming_queue.size()));

        // Reset counters for the next 1-second window
        s_window_sent    = 0;
        s_window_dropped = 0;
        s_window_bytes   = 0;
        s_last_report_us = now_us;
    }

    return 0;
}
    std::vector<uint8_t> poll_and_receive() {
        std::lock_guard<std::mutex> lock(s_rx_mutex);
        if (s_rx_queue.empty()) {
            return {};
        }

        std::vector<uint8_t> data = std::move(s_rx_queue.front());
        s_rx_queue.pop();
        return data;
    }

    void update(uint32_t /*current_ms*/) {
        // No-op: Ticking is managed internally inside network_worker_task
    }

    // =========================================================================
    // Internal Autonomous Handshake Engine
    // =========================================================================

    static void handle_autonomous_handshake(uint32_t now_ms) {
        if (!wifi::isConnected()) {
            if (s_hs_state != HandshakeState::IDLE) {
                ESP_LOGW(TAG, "Wi-Fi link lost: Resetting handshake state to IDLE");
                s_hs_state = HandshakeState::IDLE;
                s_cached_handshake_pkt.clear();
                s_handshake_retry_count = 0;
            }
            return;
        }

        switch (s_hs_state) {
            case HandshakeState::IDLE: {
                ESP_LOGI(TAG, "Wi-Fi ready: Generating ECDH local keypair...");
                auto my_key_opt = encryption::ecdh_step1_generate_my_key();
                if (!my_key_opt.has_value()) {
                    ESP_LOGE(TAG, "Key generation failed; will retry next tick");
                    return;
                }

                const auto& pub_key_bytes = my_key_opt.value();
                s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());

                // Packet Format: [conv_id (4B)] + [Client Public Key]
                std::memcpy(s_cached_handshake_pkt.data(), &s_conv_id, sizeof(uint32_t));
                std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

                s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
                s_last_handshake_attempt_ms = now_ms;
                s_handshake_retry_count = 1;
                s_hs_state = HandshakeState::WAIT_ACK;

                ESP_LOGI(TAG, "Handshake dispatched (conv_id: %u, attempt #1)", s_conv_id);
                break;
            }

            case HandshakeState::WAIT_ACK: {
                uint32_t retry_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
                if (retry_interval > MAX_HANDSHAKE_RETRY_MS) {
                    retry_interval = MAX_HANDSHAKE_RETRY_MS;
                }

                if (now_ms - s_last_handshake_attempt_ms >= retry_interval) {
                    s_handshake_retry_count++;
                    ESP_LOGW(TAG, "Handshake ACK timeout; retransmitting attempt #%u...", s_handshake_retry_count);
                    s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
                    s_last_handshake_attempt_ms = now_ms;
                }
                break;
            }

            case HandshakeState::ESTABLISHED:
                break;
        }
    }
static void network_worker_task(void* /*pvParameters*/) {
        ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

        // --- Performance Profiling Timers (Aggregated & logged every 1s) ---
        int64_t  perf_window_start_us = esp_timer_get_time();
        uint32_t stat_loop_count      = 0;
        uint32_t stat_sent_pkts       = 0;
        int64_t  stat_work_time_us    = 0;
        int64_t  stat_crypto_time_us  = 0;
        int64_t  stat_kcp_time_us     = 0;

        // Tracks task wake time for drift-free 10ms intervals
        TickType_t last_wake_time = xTaskGetTickCount();

        while (s_worker_running.load()) {
            int64_t loop_start_us = esp_timer_get_time();
            uint32_t now_ms = static_cast<uint32_t>(loop_start_us / 1000ULL);

            // 1. Maintain Handshake State Machine
            handle_autonomous_handshake(now_ms);

            // 2. KCP Clock Update & Inbound Processing
            int64_t t_kcp_start = esp_timer_get_time();
            if (s_kcp_client) {
                s_kcp_client->update(now_ms);

                // 3. Receive Packets & Process Crypto Responses
                auto incoming_data = s_kcp_client->poll_and_receive();

                if (!incoming_data.empty()) {
                    if (s_hs_state == HandshakeState::WAIT_ACK) {
                        auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
                        if (shared_secret.has_value()) {
                            s_aes_key = shared_secret.value();
                            s_hs_state = HandshakeState::ESTABLISHED;
                            s_cached_handshake_pkt.clear();
                            ESP_LOGI(TAG, "Handshake completed! Session AES-GCM key established.");
                        } else {
                            ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
                        }
                    } else if (s_hs_state == HandshakeState::ESTABLISHED) {
                        int64_t t_crypto = esp_timer_get_time();
                        auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
                        stat_crypto_time_us += (esp_timer_get_time() - t_crypto);

                        if (decrypted.has_value()) {
                            std::lock_guard<std::mutex> lock(s_rx_mutex);
                            s_rx_queue.push(std::move(decrypted.value()));
                        } else {
                            ESP_LOGE(TAG, "Failed to decrypt incoming packet");
                        }
                    }
                }
            }
            stat_kcp_time_us += (esp_timer_get_time() - t_kcp_start);

            // 4. Swap Producer Buffer into Worker Buffer
            {
                std::lock_guard<std::mutex> lock(s_tx_mutex);
                if (!s_incoming_queue.empty()) {
                    s_sending_queue.swap(s_incoming_queue);
                }
            }

            // 5. Encrypt & Transmit
            bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

            if (!s_sending_queue.empty()) {
                if (is_online && s_kcp_client) {
                    for (const auto& raw_packet : s_sending_queue) {
                        // Measure AES encryption duration
                        int64_t t_crypto = esp_timer_get_time();
                        auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
                        stat_crypto_time_us += (esp_timer_get_time() - t_crypto);

                        if (encrypted.has_value()) {
                            // Measure KCP send duration
                            int64_t t_send = esp_timer_get_time();
                            int res = s_kcp_client->send(encrypted->data(), encrypted->size());
                            stat_kcp_time_us += (esp_timer_get_time() - t_send);

                            if (res == -1) {
                                ESP_LOGW(TAG, "KCP send buffer saturated, dropping packet");
                            } else {
                                stat_sent_pkts++;
                            }
                        } else {
                            ESP_LOGE(TAG, "Failed to encrypt packet");
                        }
                    }
                } else {
                    ESP_LOGW(TAG, "Dropped %u packet(s): Connection not established", 
                             static_cast<unsigned>(s_sending_queue.size()));
                }
                s_sending_queue.clear();
            }

            int64_t loop_end_us = esp_timer_get_time();
            stat_work_time_us += (loop_end_us - loop_start_us);
            stat_loop_count++;

            // =================================================================
            // 1-Second Periodic Performance Report
            // =================================================================
            if (loop_end_us - perf_window_start_us >= 1000000ULL) {
                // Log only when active to avoid flooding the console when idle
                if (stat_sent_pkts > 0 || stat_work_time_us > 10000ULL) {
                    double cpu_duty = (stat_work_time_us / 1000000.0) * 100.0;
                    ESP_LOGI(TAG, "[1s PERF] Loops: %u | Sent: %u | Active: %lld us (Duty: %.1f%%) | KCP: %lld us | Crypto: %lld us",
                             stat_loop_count, stat_sent_pkts, stat_work_time_us, cpu_duty, stat_kcp_time_us, stat_crypto_time_us);

                             // Add this inside the 1-second PERF block in network_worker_task:
size_t free_dma   = heap_caps_get_free_size(MALLOC_CAP_DMA);
size_t max_dma    = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
size_t total_free = esp_get_free_heap_size();

ESP_LOGI(TAG, "[RAM 1s] Total Free: %u B | DMA Free: %u B | Largest DMA Block: %u B",
         (unsigned)total_free, (unsigned)free_dma, (unsigned)max_dma);
                }

                // Reset statistics for the next 1-second window
                stat_loop_count      = 0;
                stat_sent_pkts       = 0;
                stat_work_time_us    = 0;
                stat_crypto_time_us  = 0;
                stat_kcp_time_us     = 0;
                perf_window_start_us = esp_timer_get_time();
            }

            // 6. Cadence: Fixed 10ms intervals without tick drift
            vTaskDelayUntil(&last_wake_time, WORKER_TICK_DELAY);
        }

        s_worker_task_handle = nullptr;
        vTaskDelete(nullptr);
    }
} // namespace Network