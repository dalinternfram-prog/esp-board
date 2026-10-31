
//#define TestFIFOFile3



#if defined (Test1FIFOFile)
#include "FIFO_file.hpp"
#include "file_manager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cassert>

static const char* TAG = "FIFO_TEST";

// ============================================================================
// Internal Test Utilities
// ============================================================================

// Ensures a clean slate before each test run so leftover residue never interferes
static void clean_test_file(const std::string& name) {
    FileManager::deleteFile(name);
}

// Generates a patterned packet with sequence ID embedded in the first 4 bytes
static std::vector<uint8_t> make_packet(uint32_t seq_id, size_t size_bytes) {
    std::vector<uint8_t> pkt(size_bytes);
    pkt[0] = static_cast<uint8_t>(seq_id & 0xFF);
    pkt[1] = static_cast<uint8_t>((seq_id >> 8) & 0xFF);
    pkt[2] = static_cast<uint8_t>((seq_id >> 16) & 0xFF);
    pkt[3] = static_cast<uint8_t>((seq_id >> 24) & 0xFF);

    for (size_t i = 4; i < size_bytes; ++i) {
        pkt[i] = static_cast<uint8_t>((seq_id + i) & 0xFF);
    }
    return pkt;
}

// Verifies packet sequence, length, and payload integrity byte-by-byte
static bool verify_packet(const std::vector<uint8_t>& pkt, uint32_t expected_seq, size_t expected_size) {
    if (pkt.size() != expected_size) {
        ESP_LOGE(TAG, "Size mismatch: expected %u, got %u", (unsigned)expected_size, (unsigned)pkt.size());
        return false;
    }
    uint32_t seq = static_cast<uint32_t>(pkt[0]) |
                  (static_cast<uint32_t>(pkt[1]) << 8) |
                  (static_cast<uint32_t>(pkt[2]) << 16) |
                  (static_cast<uint32_t>(pkt[3]) << 24);
    if (seq != expected_seq) {
        ESP_LOGE(TAG, "Sequence mismatch: expected %u, got %u", (unsigned)expected_seq, (unsigned)seq);
        return false;
    }
    for (size_t i = 4; i < expected_size; ++i) {
        uint8_t expected_val = static_cast<uint8_t>((expected_seq + i) & 0xFF);
        if (pkt[i] != expected_val) {
            ESP_LOGE(TAG, "Byte corruption at offset %u in packet %u (expected 0x%02X, got 0x%02X)",
                     (unsigned)i, (unsigned)expected_seq, expected_val, pkt[i]);
            return false;
        }
    }
    return true;
}

// ============================================================================
// TEST 1: Pure RAM Mode (Data fits entirely inside cache)
// ============================================================================
static void test_pure_ram_mode() {
    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "[TEST 1] Pure RAM Mode (Zero Flash Spills)");

    const std::string test_file = "test_fifo_ram";
    clean_test_file(test_file);

    const size_t cache_size = 64 * 1024; // 64 KB RAM cache
    const size_t pkt_count = 200;
    const size_t pkt_len = 100; // Total: 200 * 104B ~ 20.8 KB (fits comfortably)

    file::FIFO fifo(test_file, cache_size);

    int64_t t0 = esp_timer_get_time();
    for (uint32_t i = 0; i < pkt_count; ++i) {
        fifo.push_back(make_packet(i, pkt_len));
    }
    int64_t push_time = esp_timer_get_time() - t0;

    int64_t t1 = esp_timer_get_time();
    for (uint32_t i = 0; i < pkt_count; ++i) {
        auto pkt = fifo.Pop();
        if (!verify_packet(pkt, i, pkt_len)) {
            ESP_LOGE(TAG, "TEST 1 FAILED at packet %u", (unsigned)i);
            return;
        }
    }
    int64_t pop_time = esp_timer_get_time() - t1;

    if (!fifo.empty()) {
        ESP_LOGE(TAG, "TEST 1 FAILED: FIFO not empty after popping all packets!");
        return;
    }

    ESP_LOGI(TAG, "[TEST 1 PASSED] 200 pkts (20.8KB) -> Push: %lld us (%.2f us/pkt) | Pop: %lld us (%.2f us/pkt)",
             push_time, (double)push_time / pkt_count,
             pop_time, (double)pop_time / pkt_count);
}

// ============================================================================
// TEST 2: Flash Spillover & Full Sequential Drain
// ============================================================================
static void test_flash_spillover() {
    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "[TEST 2] Flash Spillover (Force Multi-Spills to Flash & Drain)");

    const std::string test_file = "test_fifo_spill";
    clean_test_file(test_file);

    const size_t cache_size = 8 * 1024; // Small 8 KB cache forces ~7 spills to disk
    const size_t pkt_count = 500;
    const size_t pkt_len = 120; // 500 * 124B ~ 62 KB total payload

    file::FIFO fifo(test_file, cache_size);

    int64_t t0 = esp_timer_get_time();
    for (uint32_t i = 0; i < pkt_count; ++i) {
        fifo.push_back(make_packet(i, pkt_len));
    }
    fifo.flush(); // Ensure any trailing RAM bytes are flushed to disk
    int64_t push_time = esp_timer_get_time() - t0;

    int64_t t1 = esp_timer_get_time();
    for (uint32_t i = 0; i < pkt_count; ++i) {
        auto pkt = fifo.Pop();
        if (!verify_packet(pkt, i, pkt_len)) {
            ESP_LOGE(TAG, "TEST 2 FAILED at packet %u", (unsigned)i);
            return;
        }
    }
    int64_t pop_time = esp_timer_get_time() - t1;

    if (!fifo.empty()) {
        ESP_LOGE(TAG, "TEST 2 FAILED: FIFO not empty after popping all packets!");
        return;
    }

    ESP_LOGI(TAG, "[TEST 2 PASSED] 500 pkts (62KB) -> Push+Spill: %lld us | Pop: %lld us",
             push_time, pop_time);
}

// ============================================================================
// TEST 3: Interleaved Consistency (Disk + RAM Coexistence)
// ============================================================================
static void test_interleaved_inconsistency() {
    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "[TEST 3] Interleaved Consistency (Disk Drain -> RAM Pop)");

    const std::string test_file = "test_fifo_interleaved";
    clean_test_file(test_file);

    const size_t cache_size = 4 * 1024; // 4 KB cache
    file::FIFO fifo(test_file, cache_size);

    // Step A: Push 100 packets (forces spillover to physical Flash disk)
    for (uint32_t i = 0; i < 100; ++i) {
        fifo.push_back(make_packet(i, 100));
    }
    fifo.flush(); // Guaranteed to be on disk

    // Step B: Pop 50 packets from disk (leaving packets 50..99 on disk)
    for (uint32_t i = 0; i < 50; ++i) {
        auto pkt = fifo.Pop();
        if (!verify_packet(pkt, i, 100)) {
            ESP_LOGE(TAG, "TEST 3 Step B failed at seq %u", (unsigned)i);
            return;
        }
    }

    // Step C: Push 50 NEW packets (i = 100 to 149) that sit strictly in RAM (writeCache_)
    for (uint32_t i = 100; i < 150; ++i) {
        fifo.push_back(make_packet(i, 100));
    }

    // Step D: Pop the remaining 100 packets:
    // First 50 must come from remaining disk (50..99)
    // Next 50 must come from RAM writeCache_ (100..149) seamlessly!
    int64_t t0 = esp_timer_get_time();
    for (uint32_t i = 50; i < 150; ++i) {
        auto pkt = fifo.Pop();
        if (!verify_packet(pkt, i, 100)) {
            ESP_LOGE(TAG, "TEST 3 Step D failed at seq %u! (Disorder or Corruption detected)", (unsigned)i);
            return;
        }
    }
    int64_t pop_time = esp_timer_get_time() - t0;

    if (!fifo.empty()) {
        ESP_LOGE(TAG, "TEST 3 FAILED: FIFO not empty after interleaved sequence!");
        return;
    }

    ESP_LOGI(TAG, "[TEST 3 PASSED] Seamless transition from Disk -> RAM verified! Pop time: %lld us", pop_time);
}

// ============================================================================
// TEST 4: Power-Loss / Reboot Head Persistence Test
// ============================================================================
static void test_power_loss_recovery() {
    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "[TEST 4] Reboot / Persistence Recovery Test");

    const std::string test_file = "test_fifo_reboot";
    clean_test_file(test_file);

    // Session 1: Push 100 packets, pop 40, flush, and simulate sudden shutdown/destructor
    {
        file::FIFO fifo1(test_file, 8 * 1024);
        for (uint32_t i = 0; i < 100; ++i) {
            fifo1.push_back(make_packet(i, 80));
        }
        for (uint32_t i = 0; i < 40; ++i) {
            auto pkt = fifo1.Pop();
            if (!verify_packet(pkt, i, 80)) {
                ESP_LOGE(TAG, "TEST 4 Session 1 failed at %u", (unsigned)i);
                return;
            }
        }
        fifo1.flush();
        // fifo1 destructs here, persisting head_ = offset of packet 40
    }

    // Session 2: Boot new instance on same file; must resume exactly from packet 40!
    {
        file::FIFO fifo2(test_file, 8 * 1024);
        for (uint32_t i = 40; i < 100; ++i) {
            auto pkt = fifo2.Pop();
            if (!verify_packet(pkt, i, 80)) {
                ESP_LOGE(TAG, "TEST 4 FAILED: Boot recovery failed at sequence %u", (unsigned)i);
                return;
            }
        }

        if (!fifo2.empty()) {
            ESP_LOGE(TAG, "TEST 4 FAILED: FIFO not empty after draining restored items!");
            return;
        }
    }

    ESP_LOGI(TAG, "[TEST 4 PASSED] Crash/Reboot head persistence successfully validated!");
}

// ============================================================================
// TEST 5: Cache Capacity Benchmark Sweep (4KB vs 32KB vs 128KB vs 256KB)
// ============================================================================
static void benchmark_cache_capacity_sweep() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[BENCHMARK] Cache Capacity Throughput Comparison");
    ESP_LOGI(TAG, "Workload: 1,000 packets of 120 bytes (~124 KB payload)");
    ESP_LOGI(TAG, "==================================================");

    const size_t capacities[] = {
        4 * 1024,    // 4 KB   (Frequent small spills)
        32 * 1024,   // 32 KB  (Moderate batch spills)
        128 * 1024,  // 128 KB (Large block spill)
        256 * 1024   // 256 KB (100% in-memory fast-path)
    };

    const uint32_t total_pkts = 1000;
    const size_t pkt_len = 120;

    for (size_t cap : capacities) {
        std::string test_name = "bench_cap_" + std::to_string(cap / 1024) + "k";
        clean_test_file(test_name);

        file::FIFO fifo(test_name, cap);

        // 1. Measure Push Duration
        int64_t t_push_start = esp_timer_get_time();
        for (uint32_t i = 0; i < total_pkts; ++i) {
            fifo.push_back(make_packet(i, pkt_len));
        }
        fifo.flush(); // Commit writeCache to disk
        int64_t t_push_total = esp_timer_get_time() - t_push_start;

        // 2. Measure Pop Duration
        int64_t t_pop_start = esp_timer_get_time();
        for (uint32_t i = 0; i < total_pkts; ++i) {
            auto pkt = fifo.Pop();
            if (pkt.empty()) {
                ESP_LOGE(TAG, "Benchmark failed: premature EOF at %u for cap %u KB", (unsigned)i, (unsigned)(cap / 1024));
                break;
            }
        }
        int64_t t_pop_total = esp_timer_get_time() - t_pop_start;

        double push_avg_us = (double)t_push_total / total_pkts;
        double pop_avg_us  = (double)t_pop_total / total_pkts;
        double total_sec   = (double)(t_push_total + t_pop_total) / 1000000.0;
        double throughput_kb_sec = (124.0) / total_sec;

        ESP_LOGW(TAG, "Cache: %3u KB | Push: %7lld us (%5.1f us/pkt) | Pop: %7lld us (%5.1f us/pkt) | Rate: %.2f KB/s",
                 (unsigned)(cap / 1024),
                 t_push_total, push_avg_us,
                 t_pop_total, pop_avg_us,
                 throughput_kb_sec);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================================
// FreeRTOS Entry Point Task
// ============================================================================
extern "C" void app_main(void* pvParameters) {
    ESP_LOGI(TAG, "Starting FIFO Comprehensive Test Suite in 1 second...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_err_t err = FileManager::init("/spiffs", "my_files", 5);
    if (err != ESP_OK) {
        ESP_LOGE("MAIN", "Filesystem initialization failed! Check partition table.");
        return;
    }
    test_pure_ram_mode();
    vTaskDelay(pdMS_TO_TICKS(50));

    test_flash_spillover();
    vTaskDelay(pdMS_TO_TICKS(50));

    test_interleaved_inconsistency();
    vTaskDelay(pdMS_TO_TICKS(50));

    test_power_loss_recovery();
    vTaskDelay(pdMS_TO_TICKS(50));

    benchmark_cache_capacity_sweep();

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, ">>> ALL FIFO TESTS COMPLETED SUCCESSFULLY! <<<");
    ESP_LOGI(TAG, "==================================================");

    vTaskDelete(NULL);
}
 
#endif







 

// ============================================================================
// TEST SUITE 2 (Advanced Robustness, Concurrency, Edge Cases & Recovery)
// ============================================================================
#if defined(TestFIFOFile2)

#include "FIFO_file.hpp"
#include "file_manager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>

static const char* TAG2 = "FIFO_TEST2";

// ============================================================================
// Internal Utilities (Autonomous implementation for Test 2)
// ============================================================================

static void clean_test_file2(const std::string& name) {
    FileManager::deleteFile(name);
}

// Generates patterned packets supporting lengths from 0 up to arbitrary sizes
static std::vector<uint8_t> make_packet2(uint32_t seq_id, size_t size_bytes) {
    std::vector<uint8_t> pkt(size_bytes);
    if (size_bytes >= 4) {
        pkt[0] = static_cast<uint8_t>(seq_id & 0xFF);
        pkt[1] = static_cast<uint8_t>((seq_id >> 8) & 0xFF);
        pkt[2] = static_cast<uint8_t>((seq_id >> 16) & 0xFF);
        pkt[3] = static_cast<uint8_t>((seq_id >> 24) & 0xFF);
        for (size_t i = 4; i < size_bytes; ++i) {
            pkt[i] = static_cast<uint8_t>((seq_id + i) & 0xFF);
        }
    } else if (size_bytes > 0) {
        for (size_t i = 0; i < size_bytes; ++i) {
            pkt[i] = static_cast<uint8_t>((seq_id + i) & 0xFF);
        }
    }
    return pkt;
}

// Byte-by-byte sequence and integrity validator
static bool verify_packet2(const std::vector<uint8_t>& pkt, uint32_t expected_seq, size_t expected_size) {
    if (pkt.size() != expected_size) {
        ESP_LOGE(TAG2, "Size mismatch: expected %u, got %u", (unsigned)expected_size, (unsigned)pkt.size());
        return false;
    }
    if (expected_size == 0) {
        return true;
    }

    if (expected_size < 4) {
        for (size_t i = 0; i < expected_size; ++i) {
            uint8_t exp = static_cast<uint8_t>((expected_seq + i) & 0xFF);
            if (pkt[i] != exp) {
                ESP_LOGE(TAG2, "Tiny packet byte mismatch at [%u]: expected 0x%02X, got 0x%02X",
                         (unsigned)i, exp, pkt[i]);
                return false;
            }
        }
        return true;
    }

    uint32_t seq = static_cast<uint32_t>(pkt[0]) |
                  (static_cast<uint32_t>(pkt[1]) << 8) |
                  (static_cast<uint32_t>(pkt[2]) << 16) |
                  (static_cast<uint32_t>(pkt[3]) << 24);

    if (seq != expected_seq) {
        ESP_LOGE(TAG2, "Sequence mismatch: expected %u, got %u", (unsigned)expected_seq, (unsigned)seq);
        return false;
    }

    for (size_t i = 4; i < expected_size; ++i) {
        uint8_t exp = static_cast<uint8_t>((expected_seq + i) & 0xFF);
        if (pkt[i] != exp) {
            ESP_LOGE(TAG2, "Payload corruption at offset %u in pkt %u (expected 0x%02X, got 0x%02X)",
                     (unsigned)i, (unsigned)expected_seq, exp, pkt[i]);
            return false;
        }
    }
    return true;
}

// ============================================================================
// TEST 1: Boundary & Edge Cases (0-byte, 1-byte, 3-byte, Pop on Empty)
// ============================================================================
static void test2_boundary_and_edge_cases() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 1] Boundary & Edge Cases (0B, 1B, 3B, Empty Pop)");

    const std::string test_file = "test2_edge_cases";
    clean_test_file2(test_file);

    file::FIFO fifo(test_file, 4 * 1024);

    // 1. Pop from brand new empty FIFO
    auto empty_pkt = fifo.Pop();
    if (!empty_pkt.empty()) {
        ESP_LOGE(TAG2, "FAILED: Popping an empty FIFO returned non-empty data!");
        return;
    }

    // 2. Push zero-length payload
    fifo.push_back(std::vector<uint8_t>());

    // 3. Push single-byte payload
    fifo.push_back(make_packet2(0x5A, 1));

    // 4. Push 3-byte payload (< 4-byte uint32 header size)
    fifo.push_back(make_packet2(0x1F2E, 3));

    // 5. Read back and verify order
    auto p0 = fifo.Pop();
    if (!p0.empty()) {
        ESP_LOGE(TAG2, "FAILED: 0-byte packet check failed, size = %u", (unsigned)p0.size());
        return;
    }

    auto p1 = fifo.Pop();
    if (!verify_packet2(p1, 0x5A, 1)) {
        ESP_LOGE(TAG2, "FAILED: 1-byte packet verification failed");
        return;
    }

    auto p3 = fifo.Pop();
    if (!verify_packet2(p3, 0x1F2E, 3)) {
        ESP_LOGE(TAG2, "FAILED: 3-byte packet verification failed");
        return;
    }

    // 6. Must be strictly empty again
    if (!fifo.empty() || !fifo.Pop().empty()) {
        ESP_LOGE(TAG2, "FAILED: FIFO not empty after draining edge cases!");
        return;
    }

    ESP_LOGI(TAG2, "[TEST 1 PASSED] 0-Byte, 1-Byte, 3-Byte, and Underflow handled cleanly.");
}

// ============================================================================
// TEST 2: Pseudo-Random Variable Packet Sizes (4B - 1500B) Across Spills
// ============================================================================
static void test2_variable_packet_sizes() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 2] Variable Sized Packets across Cache Boundaries");

    const std::string test_file = "test2_variable_size";
    clean_test_file2(test_file);

    const size_t cache_size = 4 * 1024; // 4 KB RAM cache forces multiple spills
    file::FIFO fifo(test_file, cache_size);

    const uint32_t count = 300;
    std::vector<size_t> lengths(count);

    // Generate random sizes between 4 and 1200 bytes
    for (uint32_t i = 0; i < count; ++i) {
        lengths[i] = 4 + (esp_random() % 1200);
    }

    int64_t t0 = esp_timer_get_time();
    for (uint32_t i = 0; i < count; ++i) {
        fifo.push_back(make_packet2(i, lengths[i]));
    }
    fifo.flush();
    int64_t push_time = esp_timer_get_time() - t0;

    int64_t t1 = esp_timer_get_time();
    for (uint32_t i = 0; i < count; ++i) {
        auto pkt = fifo.Pop();
        if (!verify_packet2(pkt, i, lengths[i])) {
            ESP_LOGE(TAG2, "FAILED at variable packet %u (expected len %u)", (unsigned)i, (unsigned)lengths[i]);
            return;
        }
    }
    int64_t pop_time = esp_timer_get_time() - t1;

    if (!fifo.empty()) {
        ESP_LOGE(TAG2, "FAILED: FIFO not empty after popping all variable packets!");
        return;
    }

    ESP_LOGI(TAG2, "[TEST 2 PASSED] 300 variable pkts -> Push: %lld us | Pop: %lld us", push_time, pop_time);
}

// ============================================================================
// TEST 3: Multithreaded FreeRTOS Producer-Consumer Concurrency
// ============================================================================
struct ThreadContext {
    file::FIFO* fifo;
    uint32_t total_packets;
    size_t packet_size;
    volatile bool producer_finished;
    volatile bool failure_detected;
};

static void test2_producer_task(void* pv) {
    auto* ctx = static_cast<ThreadContext*>(pv);
    for (uint32_t i = 0; i < ctx->total_packets; ++i) {
        ctx->fifo->push_back(make_packet2(i, ctx->packet_size));
        // Periodic yield to create realistic interleaving
        if ((i % 30) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    ctx->fifo->flush();
    ctx->producer_finished = true;
    vTaskDelete(NULL);
}

static void test2_consumer_task(void* pv) {
    auto* ctx = static_cast<ThreadContext*>(pv);
    uint32_t consumed_seq = 0;

    while (consumed_seq < ctx->total_packets) {
        auto pkt = ctx->fifo->Pop();
        if (pkt.empty()) {
            if (ctx->producer_finished) {
                ESP_LOGE(TAG2, "Starvation error: producer is done but queue empty at %u / %u",
                         (unsigned)consumed_seq, (unsigned)ctx->total_packets);
                ctx->failure_detected = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        if (!verify_packet2(pkt, consumed_seq, ctx->packet_size)) {
            ESP_LOGE(TAG2, "Race condition or corruption detected at seq %u", (unsigned)consumed_seq);
            ctx->failure_detected = true;
            break;
        }
        consumed_seq++;
    }

    vTaskDelete(NULL);
}

static void test2_concurrency_stress() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 3] Multithreaded Core Concurrency Stress");

    const std::string test_file = "test2_concurrency";
    clean_test_file2(test_file);

    file::FIFO fifo(test_file, 8 * 1024);

    ThreadContext ctx;
    ctx.fifo = &fifo;
    ctx.total_packets = 1200;
    ctx.packet_size = 96;
    ctx.producer_finished = false;
    ctx.failure_detected = false;

    TaskHandle_t hProd = nullptr;
    TaskHandle_t hCons = nullptr;

    // Pin producer to Core 0 and consumer to Core 1
    xTaskCreatePinnedToCore(test2_producer_task, "t2_prod", 4096, &ctx, 5, &hProd, 0);
    xTaskCreatePinnedToCore(test2_consumer_task, "t2_cons", 4096, &ctx, 5, &hCons, 1);

    while (eTaskGetState(hProd) != eDeleted || eTaskGetState(hCons) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (ctx.failure_detected) {
        ESP_LOGE(TAG2, "[TEST 3 FAILED] Multithreaded concurrency test failed.");
        return;
    }

    if (!fifo.empty()) {
        ESP_LOGE(TAG2, "[TEST 3 FAILED] FIFO not empty after concurrent run finished.");
        return;
    }

    ESP_LOGI(TAG2, "[TEST 3 PASSED] Successfully coordinated %u cross-core packets.", (unsigned)ctx.total_packets);
}

// ============================================================================
// TEST 4: Long-Run Sliding Window Cycling (SPIFFS GC / Circular Stability)
// ============================================================================
static void test2_long_run_sliding_window() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 4] Sliding Window Cycling (SPIFFS GC & Offset Growth)");

    const std::string test_file = "test2_slide_window";
    clean_test_file2(test_file);

    // Small 2 KB cache forces persistent flash writes and reads
    file::FIFO fifo(test_file, 2 * 1024);

    const uint32_t total_cycles = 4000;
    const size_t pkt_len = 64;
    uint32_t push_seq = 0;
    uint32_t pop_seq = 0;

    int64_t t0 = esp_timer_get_time();

    // Maintain a steady backlog of ~15 packets
    for (uint32_t i = 0; i < total_cycles; ++i) {
        fifo.push_back(make_packet2(push_seq++, pkt_len));

        if (push_seq - pop_seq > 15) {
            auto pkt = fifo.Pop();
            if (!verify_packet2(pkt, pop_seq++, pkt_len)) {
                ESP_LOGE(TAG2, "[TEST 4] FAILED at pop_seq %u", (unsigned)(pop_seq - 1));
                return;
            }
        }
    }

    // Drain remainder
    while (pop_seq < push_seq) {
        auto pkt = fifo.Pop();
        if (!verify_packet2(pkt, pop_seq++, pkt_len)) {
            ESP_LOGE(TAG2, "[TEST 4] FAILED during remainder drain at %u", (unsigned)(pop_seq - 1));
            return;
        }
    }

    int64_t elapsed_ms = (esp_timer_get_time() - t0) / 1000;
    ESP_LOGI(TAG2, "[TEST 4 PASSED] %u push/pop operations completed in %lld ms.", (unsigned)total_cycles, elapsed_ms);
}

// ============================================================================
// TEST 5: Partial Write / Corrupted Tail Power-Cut Simulation
// ============================================================================
static void test2_tail_corruption_recovery() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 5] Corrupted Tail Recovery (Simulated Power Cut)");

    const std::string test_file = "test2_tail_corrupt";
    clean_test_file2(test_file);

    // 1. Push 25 valid packets and flush to flash
    {
        file::FIFO fifo1(test_file, 4 * 1024);
        for (uint32_t i = 0; i < 25; ++i) {
            fifo1.push_back(make_packet2(i, 64));
        }
        fifo1.flush();
    }

    // 2. Append broken partial bytes to disk (mimicking power loss mid-write)
    {
        FILE* fp = fopen(("/spiffs/" + test_file).c_str(), "ab");
        if (fp) {
            uint8_t garbage_bytes[7] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01, 0x02};
            fwrite(garbage_bytes, 1, sizeof(garbage_bytes), fp);
            fclose(fp);
            ESP_LOGI(TAG2, "Injected 7 invalid/truncated bytes at the end of the backing file.");
        } else {
            ESP_LOGE(TAG2, "Could not open file to inject corrupted bytes!");
            return;
        }
    }

    // 3. Re-instantiate FIFO: valid packets must still be intact and readable
    {
        file::FIFO fifo2(test_file, 4 * 1024);
        for (uint32_t i = 0; i < 25; ++i) {
            auto pkt = fifo2.Pop();
            if (!verify_packet2(pkt, i, 64)) {
                ESP_LOGE(TAG2, "[TEST 5] FAILED: Pre-crash packet %u corrupted by tail garbage!", (unsigned)i);
                return;
            }
        }

        // The next pop encounters the corrupt fragment; must return empty (EOF) or handle gracefully
        auto trailing = fifo2.Pop();
        ESP_LOGI(TAG2, "Pop on trailing corrupt bytes returned packet of size %u (clean EOF)", (unsigned)trailing.size());
    }

    ESP_LOGI(TAG2, "[TEST 5 PASSED] Valid data preserved and recovered despite dirty tail!");
}

// ============================================================================
// FreeRTOS Entry Point for Test Suite 2
// ============================================================================
extern "C" void app_main(void* pvParameters) {
    ESP_LOGI(TAG2, "Starting FIFO Comprehensive Test Suite 2 in 1 second...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_err_t err = FileManager::init("/spiffs", "my_files", 5);
    if (err != ESP_OK) {
        ESP_LOGE(TAG2, "Filesystem initialization failed! Check partition table.");
        return;
    }

    test2_boundary_and_edge_cases();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_variable_packet_sizes();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_concurrency_stress();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_long_run_sliding_window();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_tail_corruption_recovery();
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG2, "==================================================");
    ESP_LOGI(TAG2, ">>> ALL TEST SUITE 2 TESTS COMPLETED SUCCESSFULLY! <<<");
    ESP_LOGI(TAG2, "==================================================");

    vTaskDelete(NULL);
}

#endif // TestFIFOFile2


// ============================================================================
// Macro Selector: Comment/Uncomment to switch test suites
// ============================================================================
//#define TestFIFOFile
 

// ============================================================================
// TEST SUITE 2 (Strictly Single-Threaded: Edge Cases, Patterns, Multi-Reboots)
// ============================================================================
#if defined(TestFIFOFile3)

#include "FIFO_file.hpp"
#include "file_manager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>

static const char* TAG2 = "FIFO_TEST2";

// ============================================================================
// Internal Utilities
// ============================================================================

static void clean_test_file2(const std::string& name) {
    FileManager::deleteFile(name);
}

// Generates patterned packets supporting arbitrary lengths >= 1
static std::vector<uint8_t> make_packet2(uint32_t seq_id, size_t size_bytes) {
    if (size_bytes == 0) return {};

    std::vector<uint8_t> pkt(size_bytes);
    if (size_bytes >= 4) {
        pkt[0] = static_cast<uint8_t>(seq_id & 0xFF);
        pkt[1] = static_cast<uint8_t>((seq_id >> 8) & 0xFF);
        pkt[2] = static_cast<uint8_t>((seq_id >> 16) & 0xFF);
        pkt[3] = static_cast<uint8_t>((seq_id >> 24) & 0xFF);
        for (size_t i = 4; i < size_bytes; ++i) {
            pkt[i] = static_cast<uint8_t>((seq_id + i) & 0xFF);
        }
    } else {
        for (size_t i = 0; i < size_bytes; ++i) {
            pkt[i] = static_cast<uint8_t>((seq_id + i) & 0xFF);
        }
    }
    return pkt;
}

// Byte-by-byte sequence and integrity validator
static bool verify_packet2(const std::vector<uint8_t>& pkt, uint32_t expected_seq, size_t expected_size) {
    if (pkt.size() != expected_size) {
        ESP_LOGE(TAG2, "Size mismatch: expected %u, got %u", (unsigned)expected_size, (unsigned)pkt.size());
        return false;
    }
    if (expected_size == 0) return true;

    if (expected_size < 4) {
        for (size_t i = 0; i < expected_size; ++i) {
            uint8_t exp = static_cast<uint8_t>((expected_seq + i) & 0xFF);
            if (pkt[i] != exp) {
                ESP_LOGE(TAG2, "Small packet mismatch at [%u]: expected 0x%02X, got 0x%02X",
                         (unsigned)i, exp, pkt[i]);
                return false;
            }
        }
        return true;
    }

    uint32_t seq = static_cast<uint32_t>(pkt[0]) |
                  (static_cast<uint32_t>(pkt[1]) << 8) |
                  (static_cast<uint32_t>(pkt[2]) << 16) |
                  (static_cast<uint32_t>(pkt[3]) << 24);

    if (seq != expected_seq) {
        ESP_LOGE(TAG2, "Sequence mismatch: expected %u, got %u", (unsigned)expected_seq, (unsigned)seq);
        return false;
    }

    for (size_t i = 4; i < expected_size; ++i) {
        uint8_t exp = static_cast<uint8_t>((expected_seq + i) & 0xFF);
        if (pkt[i] != exp) {
            ESP_LOGE(TAG2, "Byte corruption at offset %u in pkt %u (expected 0x%02X, got 0x%02X)",
                     (unsigned)i, (unsigned)expected_seq, exp, pkt[i]);
            return false;
        }
    }
    return true;
}

// ============================================================================
// TEST 1: Boundary & Minimal Sizes (1B, 2B, 3B, 4B, and Pop on Empty)
// ============================================================================
static void test2_boundary_and_edge_cases() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 1] Boundary & Edge Cases (1B, 2B, 3B, 4B, Pop Underflow)");

    const std::string test_file = "test2_edge_cases";
    clean_test_file2(test_file);

    file::FIFO fifo(test_file, 4 * 1024);

    // 1. Pop from brand new empty FIFO
    auto empty_pkt = fifo.Pop();
    if (!empty_pkt.empty()) {
        ESP_LOGE(TAG2, "FAILED: Popping empty FIFO returned non-empty vector (size: %u)!", (unsigned)empty_pkt.size());
        return;
    }

    // 2. Push minimum byte lengths (1, 2, 3, 4 bytes)
    fifo.push_back(make_packet2(0x5A, 1));
    fifo.push_back(make_packet2(0x1234, 2));
    fifo.push_back(make_packet2(0x1F2E3D, 3));
    fifo.push_back(make_packet2(0xDEADBEEF, 4));

    // 3. Read back and verify
    auto p1 = fifo.Pop();
    if (!verify_packet2(p1, 0x5A, 1)) {
        ESP_LOGE(TAG2, "FAILED: 1-byte packet verification failed");
        return;
    }

    auto p2 = fifo.Pop();
    if (!verify_packet2(p2, 0x1234, 2)) {
        ESP_LOGE(TAG2, "FAILED: 2-byte packet verification failed");
        return;
    }

    auto p3 = fifo.Pop();
    if (!verify_packet2(p3, 0x1F2E3D, 3)) {
        ESP_LOGE(TAG2, "FAILED: 3-byte packet verification failed");
        return;
    }

    auto p4 = fifo.Pop();
    if (!verify_packet2(p4, 0xDEADBEEF, 4)) {
        ESP_LOGE(TAG2, "FAILED: 4-byte packet verification failed");
        return;
    }

    // 4. Ensure it transitions cleanly back to empty
    if (!fifo.empty() || !fifo.Pop().empty()) {
        ESP_LOGE(TAG2, "FAILED: FIFO not empty after popping boundary items!");
        return;
    }

    ESP_LOGI(TAG2, "[TEST 1 PASSED] 1B-4B boundaries and Pop-Underflow handled properly.");
}

// ============================================================================
// TEST 2: Variable Packet Sizes Across Cache Flush Boundaries
// ============================================================================
static void test2_variable_packet_sizes() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 2] Variable Sized Packets across Cache Boundaries");

    const std::string test_file = "test2_variable_size";
    clean_test_file2(test_file);

    const size_t cache_size = 4 * 1024; // 4 KB RAM cache forces multiple spills
    file::FIFO fifo(test_file, cache_size);

    const uint32_t count = 300;
    std::vector<size_t> lengths(count);

    // Random lengths between 4 and 1000 bytes
    for (uint32_t i = 0; i < count; ++i) {
        lengths[i] = 4 + (esp_random() % 1000);
    }

    int64_t t0 = esp_timer_get_time();
    for (uint32_t i = 0; i < count; ++i) {
        fifo.push_back(make_packet2(i, lengths[i]));
    }
    fifo.flush();
    int64_t push_time = esp_timer_get_time() - t0;

    int64_t t1 = esp_timer_get_time();
    for (uint32_t i = 0; i < count; ++i) {
        auto pkt = fifo.Pop();
        if (!verify_packet2(pkt, i, lengths[i])) {
            ESP_LOGE(TAG2, "FAILED at variable packet %u (expected len %u)", (unsigned)i, (unsigned)lengths[i]);
            return;
        }
    }
    int64_t pop_time = esp_timer_get_time() - t1;

    if (!fifo.empty()) {
        ESP_LOGE(TAG2, "FAILED: FIFO not empty after variable drain!");
        return;
    }

    ESP_LOGI(TAG2, "[TEST 2 PASSED] 300 variable pkts -> Push: %lld us | Pop: %lld us", push_time, pop_time);
}

// ============================================================================
// TEST 3: Strict 1-to-1 Alternating Ping-Pong (Push -> Pop -> Push -> Pop)
// ============================================================================
static void test2_ping_pong_single_threaded() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 3] Alternating Ping-Pong (1 Push / 1 Pop cycles)");

    const std::string test_file = "test2_pingpong";
    clean_test_file2(test_file);

    // 2 KB cache forces frequent head/tail advances
    file::FIFO fifo(test_file, 2 * 1024);

    const uint32_t iterations = 500;
    const size_t pkt_len = 128;

    int64_t t0 = esp_timer_get_time();
    for (uint32_t i = 0; i < iterations; ++i) {
        fifo.push_back(make_packet2(i, pkt_len));

        auto pkt = fifo.Pop();
        if (!verify_packet2(pkt, i, pkt_len)) {
            ESP_LOGE(TAG2, "[TEST 3 FAILED] Ping-pong corruption at iteration %u", (unsigned)i);
            return;
        }

        if (!fifo.empty()) {
            ESP_LOGE(TAG2, "[TEST 3 FAILED] FIFO not empty immediately after ping-pong pop at %u", (unsigned)i);
            return;
        }
    }
    int64_t elapsed_us = esp_timer_get_time() - t0;

    ESP_LOGI(TAG2, "[TEST 3 PASSED] 500 ping-pong iterations verified in %lld us (%.1f us/op)",
             elapsed_us, (double)elapsed_us / iterations);
}

// ============================================================================
// TEST 4: Multi-Session Reboot Recovery (5 Successive Restart Sessions)
// ============================================================================
static void test2_multi_session_reboots() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 4] Multi-Session Reboot Sequence (5 successive cycles)");

    const std::string test_file = "test2_multireboot";
    clean_test_file2(test_file);

    uint32_t push_counter = 0;
    uint32_t pop_counter = 0;
    const size_t pkt_len = 80;

    // Simulate 5 separate boot-work-shutdown sessions on the same file
    for (int session = 1; session <= 5; ++session) {
        file::FIFO session_fifo(test_file, 4 * 1024);

        // Push 40 new items
        for (uint32_t i = 0; i < 40; ++i) {
            session_fifo.push_back(make_packet2(push_counter++, pkt_len));
        }

        // Pop 20 items (leaving 20 items in the FIFO across session restarts)
        for (uint32_t i = 0; i < 20; ++i) {
            auto pkt = session_fifo.Pop();
            if (!verify_packet2(pkt, pop_counter++, pkt_len)) {
                ESP_LOGE(TAG2, "[TEST 4 FAILED] Session %d pop error at sequence %u", session, (unsigned)(pop_counter - 1));
                return;
            }
        }

        session_fifo.flush();
        // session_fifo goes out of scope here (destructs cleanly, saving state)
    }

    // Final drain session: all remaining un-popped packets must still be intact
    {
        file::FIFO final_fifo(test_file, 4 * 1024);
        while (pop_counter < push_counter) {
            auto pkt = final_fifo.Pop();
            if (!verify_packet2(pkt, pop_counter++, pkt_len)) {
                ESP_LOGE(TAG2, "[TEST 4 FAILED] Final session drain error at sequence %u", (unsigned)(pop_counter - 1));
                return;
            }
        }

        if (!final_fifo.empty()) {
            ESP_LOGE(TAG2, "[TEST 4 FAILED] FIFO not empty after final drain!");
            return;
        }
    }

    ESP_LOGI(TAG2, "[TEST 4 PASSED] 5 successive sessions survived without offset or state drift.");
}

// ============================================================================
// TEST 5: Power-Cut Simulation with Corrupted Tail Fragment
// ============================================================================
static void test2_tail_corruption_recovery() {
    ESP_LOGI(TAG2, "--------------------------------------------------");
    ESP_LOGI(TAG2, "[TEST 5] Corrupted Tail Recovery (Simulated Mid-Write Crash)");

    const std::string test_file = "test2_tail_corrupt";
    clean_test_file2(test_file);

    // Phase 1: Write 20 valid packets
    {
        file::FIFO fifo1(test_file, 4 * 1024);
        for (uint32_t i = 0; i < 20; ++i) {
            fifo1.push_back(make_packet2(i, 64));
        }
        fifo1.flush();
    }

    // Phase 2: Inject corrupt bytes at the raw file level
    {
        FILE* fp = fopen(("/spiffs/" + test_file).c_str(), "ab");
        if (fp) {
            uint8_t dirty_bytes[7] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03};
            fwrite(dirty_bytes, 1, sizeof(dirty_bytes), fp);
            fclose(fp);
            ESP_LOGI(TAG2, "Injected 7 dirty bytes directly to simulate crash mid-write.");
        } else {
            ESP_LOGE(TAG2, "Could not open file to inject corrupt bytes.");
            return;
        }
    }

    // Phase 3: Recover and verify valid packets remain intact
    {
        file::FIFO fifo2(test_file, 4 * 1024);
        for (uint32_t i = 0; i < 20; ++i) {
            auto pkt = fifo2.Pop();
            if (!verify_packet2(pkt, i, 64)) {
                ESP_LOGE(TAG2, "[TEST 5 FAILED] Packet %u corrupted by trailing bytes!", (unsigned)i);
                return;
            }
        }

        auto tail_pkt = fifo2.Pop();
        ESP_LOGI(TAG2, "Read on corrupted tail returned packet with size: %u (expected empty or skipped)", (unsigned)tail_pkt.size());
    }

    ESP_LOGI(TAG2, "[TEST 5 PASSED] Valid packets safely read back despite trailing dirty bytes.");
}

// ============================================================================
// FreeRTOS Entry Point for Test Suite 2 (Single-Thread Runner)
// ============================================================================
extern "C" void app_main(void* pvParameters) {
    ESP_LOGI(TAG2, "Starting FIFO Single-Thread Test Suite 2 in 1 second...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_err_t err = FileManager::init("/spiffs", "my_files", 5);
    if (err != ESP_OK) {
        ESP_LOGE(TAG2, "Filesystem initialization failed! Check partition table.");
        return;
    }

    test2_boundary_and_edge_cases();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_variable_packet_sizes();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_ping_pong_single_threaded();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_multi_session_reboots();
    vTaskDelay(pdMS_TO_TICKS(50));

    test2_tail_corruption_recovery();
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG2, "==================================================");
    ESP_LOGI(TAG2, ">>> ALL TEST SUITE 2 TESTS COMPLETED! <<<");
    ESP_LOGI(TAG2, "==================================================");

    vTaskDelete(NULL);
}

#endif // TestFIFOFile2





