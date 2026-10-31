//#define StressTestSingleFile
#if defined (StressTestSingleFile)
#include "single_file.hpp"
#include "file_manager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <cstdlib>

static const char* TAG = "SF_HEAVY_STRESS";

// Simple fast rolling 32-bit checksum for payload verification without heavy RAM usage
static uint32_t compute_checksum(const uint8_t* data, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

// Generate structured packet with sequence number, size, payload, and CRC
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t magic;      // 0xDEADBEEF
    uint32_t seq;        // Packet sequence number
    uint32_t length;     // Data length
    uint32_t checksum;   // Checksum of payload
};
#pragma pack(pop)

static std::vector<uint8_t> create_packet(uint32_t seq, size_t payload_len) {
    std::vector<uint8_t> packet(sizeof(PacketHeader) + payload_len);
    PacketHeader* hdr = reinterpret_cast<PacketHeader*>(packet.data());
    hdr->magic = 0xDEADBEEF;
    hdr->seq = seq;
    hdr->length = static_cast<uint32_t>(payload_len);

    uint8_t* payload = packet.data() + sizeof(PacketHeader);
    for (size_t i = 0; i < payload_len; ++i) {
        payload[i] = static_cast<uint8_t>((seq + i * 7) & 0xFF);
    }
    hdr->checksum = compute_checksum(payload, payload_len);
    return packet;
}

static bool verify_packet(const uint8_t* data, size_t total_len, uint32_t expected_seq) {
    if (total_len < sizeof(PacketHeader)) return false;
    const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(data);
    if (hdr->magic != 0xDEADBEEF) {
        ESP_LOGE(TAG, "Bad magic: 0x%08X", (unsigned)hdr->magic);
        return false;
    }
    if (hdr->seq != expected_seq) {
        ESP_LOGE(TAG, "Seq mismatch! Expected: %u, Got: %u", (unsigned)expected_seq, (unsigned)hdr->seq);
        return false;
    }
    if (sizeof(PacketHeader) + hdr->length != total_len) {
        ESP_LOGE(TAG, "Length mismatch in header: %u, total: %zu", (unsigned)hdr->length, total_len);
        return false;
    }
    const uint8_t* payload = data + sizeof(PacketHeader);
    uint32_t chk = compute_checksum(payload, hdr->length);
    if (chk != hdr->checksum) {
        ESP_LOGE(TAG, "Checksum mismatch on seq %u! Expected: 0x%08X, Computed: 0x%08X", 
                 (unsigned)hdr->seq, (unsigned)hdr->checksum, (unsigned)chk);
        return false;
    }
    return true;
}

// ============================================================================
// STRESS TEST 1: FIFO Queue Shift & Truncation Under Heavy Pressure
// Simulates 200 consecutive Push & Pop cycles using shiftBytes & truncate.
// ============================================================================
static void stress_test_fifo_queue_compaction() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 1] FIFO Push/Pop & shiftBytes Compaction Endurance");

    const std::string filename = "stress_fifo_test";
    const size_t PACKET_PAYLOAD = 256;
    const size_t PACKET_TOTAL = sizeof(PacketHeader) + PACKET_PAYLOAD; // 272 bytes
    const size_t QUEUE_CAPACITY = 10; // Keep ~10 packets queued
    const size_t TOTAL_TRANSACTIONS = 200; // 200 pops and pushes

    uint32_t push_seq = 0;
    uint32_t pop_seq = 0;

    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0); // Clean start

        ESP_LOGI(TAG, "Pre-filling FIFO with %zu packets...", QUEUE_CAPACITY);
        for (size_t i = 0; i < QUEUE_CAPACITY; ++i) {
            auto pkt = create_packet(push_seq++, PACKET_PAYLOAD);
            sf.writeFile(pkt, -1);
        }
        assert(sf.fileSize() == QUEUE_CAPACITY * PACKET_TOTAL);

        int64_t t_start = esp_timer_get_time();
        int64_t max_pop_us = 0;

        for (size_t iter = 0; iter < TOTAL_TRANSACTIONS; ++iter) {
            // 1. Peek & Verify Front Packet
            auto front = sf.readFileBytes(0, PACKET_TOTAL);
            assert(front.size() == PACKET_TOTAL);
            bool ok = verify_packet(front.data(), front.size(), pop_seq);
            if (!ok) {
                ESP_LOGE(TAG, "FIFO corruption detected at iteration %zu (seq %u)", iter, (unsigned)pop_seq);
                assert(false);
            }
            pop_seq++;

            // 2. Pop Front by shifting remaining file left and truncating
            int64_t t0 = esp_timer_get_time();
            size_t current_sz = sf.fileSize();
            sf.shiftBytes(PACKET_TOTAL, /*left=*/true, PACKET_TOTAL, current_sz);
            sf.truncate(current_sz - PACKET_TOTAL);
            int64_t pop_duration = esp_timer_get_time() - t0;
            if (pop_duration > max_pop_us) max_pop_us = pop_duration;

            // 3. Push New Packet at Tail
            auto new_pkt = create_packet(push_seq++, PACKET_PAYLOAD);
            sf.writeFile(new_pkt, -1);

            // Yield periodically to feed FreeRTOS watchdog
            if (iter % 25 == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
                ESP_LOGI(TAG, "Completed %zu / %zu FIFO transactions (Last pop: %lld us)", 
                         iter, TOTAL_TRANSACTIONS, pop_duration);
            }
        }

        int64_t total_time = esp_timer_get_time() - t_start;
        double avg_trans_ms = (double)(total_time / 1000) / TOTAL_TRANSACTIONS;

        ESP_LOGW(TAG, "FIFO Test Completed: %zu pops+pushes in %lld ms | Avg: %.2f ms/tx | Max Pop: %lld us",
                 TOTAL_TRANSACTIONS, total_time / 1000, avg_trans_ms, max_pop_us);

        sf.deleteFile();
    }

    ESP_LOGI(TAG, ">>> STRESS TEST 1 PASSED: Zero corruption during continuous queue compaction! <<<");
}

// ============================================================================
// STRESS TEST 2: Fuzzing / Shadow Buffer Random Access Test
// Performs 300 pseudo-random writes, partial overwrites, and reads while
// verifying against an in-memory shadow vector.
// ============================================================================
static void stress_test_random_access_shadow_mirror() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 2] Random Scatter-Gather R/W vs In-Memory Shadow");

    const std::string filename = "stress_shadow_test";
    const size_t NUM_OPS = 300;
    const size_t INITIAL_SIZE = 16 * 1024; // 16 KB initial file

    std::vector<uint8_t> shadow(INITIAL_SIZE, 0x5A);

    {
        FileManager::SingleFile sf(filename);
        sf.writeFile(shadow, 0);

        for (size_t op = 0; op < NUM_OPS; ++op) {
            int action = rand() % 3; // 0 = Overwrite, 1 = Append, 2 = Read & Verify

            if (action == 0) {
                // Random Overwrite
                if (shadow.empty()) continue;
                size_t offset = rand() % shadow.size();
                size_t write_len = 1 + (rand() % 512);
                if (offset + write_len > shadow.size()) {
                    write_len = shadow.size() - offset;
                }

                std::vector<uint8_t> chunk(write_len);
                for (size_t i = 0; i < write_len; ++i) chunk[i] = static_cast<uint8_t>(rand() & 0xFF);

                // Apply to file and shadow
                bool w_ok = sf.writeFile(chunk, static_cast<long>(offset));
                assert(w_ok);
                std::memcpy(shadow.data() + offset, chunk.data(), write_len);

            } else if (action == 1) {
                // Random Append
                size_t append_len = 1 + (rand() % 1024);
                // Keep max file size <= 64 KB to respect RAM limits
                if (shadow.size() + append_len > 64 * 1024) continue;

                std::vector<uint8_t> chunk(append_len);
                for (size_t i = 0; i < append_len; ++i) chunk[i] = static_cast<uint8_t>(rand() & 0xFF);

                bool w_ok = sf.writeFile(chunk, -1);
                assert(w_ok);
                shadow.insert(shadow.end(), chunk.begin(), chunk.end());

            } else {
                // Random Slice Verification
                if (shadow.empty()) continue;
                size_t offset = rand() % shadow.size();
                size_t read_len = 1 + (rand() % 512);
                if (offset + read_len > shadow.size()) {
                    read_len = shadow.size() - offset;
                }

                auto file_data = sf.readFileBytes(static_cast<long>(offset), static_cast<long>(read_len));
                assert(file_data.size() == read_len);
                if (std::memcmp(file_data.data(), shadow.data() + offset, read_len) != 0) {
                    ESP_LOGE(TAG, "Shadow mismatch at op %zu! Offset: %zu, Len: %zu", op, offset, read_len);
                    assert(false && "Flash data diverged from shadow mirror!");
                }
            }

            // Verify file size matches shadow
            assert(sf.fileSize() == shadow.size());

            if (op % 50 == 0) {
                vTaskDelay(pdMS_TO_TICKS(5));
                ESP_LOGI(TAG, "Fuzz progress: %zu / %zu ops (Current size: %u bytes)", 
                         op, NUM_OPS, (unsigned)shadow.size());
            }
        }

        sf.deleteFile();
    }

    ESP_LOGI(TAG, ">>> STRESS TEST 2 PASSED: 300 random scatter/gather ops verified 100%% bit-exact! <<<");
}

// ============================================================================
// STRESS TEST 3: Sustained High-Throughput Write / Read (256 KB)
// Checks LittleFS allocation boundaries, chunk streaming, and Flash wear.
// ============================================================================
static void stress_test_large_file_streaming() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 3] 256 KB Sustained Stream & Rolling Hash Verification");

    const std::string filename = "stress_large_stream";
    const size_t TOTAL_BYTES = 256 * 1024; // 256 KB
    const size_t CHUNK_SIZE = 4096;        // 4 KB buffer chunks

    uint32_t expected_hash = 2166136261u;

    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        // 1. Streaming Write
        int64_t t_w0 = esp_timer_get_time();
        std::vector<uint8_t> write_buf(CHUNK_SIZE);

        for (size_t offset = 0; offset < TOTAL_BYTES; offset += CHUNK_SIZE) {
            for (size_t i = 0; i < CHUNK_SIZE; ++i) {
                write_buf[i] = static_cast<uint8_t>((offset + i * 11) & 0xFF);
                expected_hash ^= write_buf[i];
                expected_hash *= 16777619u;
            }
            bool ok = sf.writeFile(write_buf, -1);
            assert(ok && "Chunk write failed");
            if ((offset / CHUNK_SIZE) % 16 == 0) {
                vTaskDelay(pdMS_TO_TICKS(5)); // Keep WDT happy
            }
        }
        sf.flush();
        int64_t t_w1 = esp_timer_get_time();

        assert(sf.fileSize() == TOTAL_BYTES);
        double write_kb = (double)TOTAL_BYTES / 1024.0;
        double write_sec = (double)(t_w1 - t_w0) / 1000000.0;
        ESP_LOGW(TAG, "Stream Write: 256 KB in %lld ms (%.2f KB/s)", (t_w1 - t_w0) / 1000, write_kb / write_sec);

        // 2. Streaming Read and Checksum
        int64_t t_r0 = esp_timer_get_time();
        uint32_t computed_hash = 2166136261u;

        for (size_t offset = 0; offset < TOTAL_BYTES; offset += CHUNK_SIZE) {
            auto read_buf = sf.readFileBytes(static_cast<long>(offset), static_cast<long>(CHUNK_SIZE));
            assert(read_buf.size() == CHUNK_SIZE);
            for (size_t i = 0; i < CHUNK_SIZE; ++i) {
                computed_hash ^= read_buf[i];
                computed_hash *= 16777619u;
            }
            if ((offset / CHUNK_SIZE) % 16 == 0) {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        int64_t t_r1 = esp_timer_get_time();

        double read_sec = (double)(t_r1 - t_r0) / 1000000.0;
        ESP_LOGW(TAG, "Stream Read: 256 KB in %lld ms (%.2f KB/s)", (t_r1 - t_r0) / 1000, write_kb / read_sec);

        // 3. Verify Hashes
        assert(computed_hash == expected_hash && "Stream checksum failure: data corrupted on disk!");

        sf.deleteFile();
    }

    ESP_LOGI(TAG, ">>> STRESS TEST 3 PASSED: 256 KB verified with bit-perfect hash! <<<");
}
// ============================================================================
// STRESS TEST 4: Deep Correctness & Edge/Boundary Conditions
// Validates EOF behavior, zero-length I/O, offset bounds, and handle persistence.
// ============================================================================
static void stress_test_correctness_and_boundaries() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 4] Correctness, Edge Cases & Handle Persistence");

    const std::string filename = "stress_boundary_test";

    // 1. Zero-byte file and empty operations
    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);
        assert(sf.fileSize() == 0);

        // Read from empty file
        auto empty_read = sf.readFileBytes(0, 100);
        assert(empty_read.empty() && "Reading an empty file must return empty vector");

        // Zero-byte read
        auto zero_read = sf.readFileBytes(0, 0);
        assert(zero_read.empty());

        // Zero-byte write
        std::vector<uint8_t> empty_buf;
        bool zw_ok = sf.writeFile(empty_buf, 0);
        assert(zw_ok);
        assert(sf.fileSize() == 0);
    }

    // 2. Exact boundary overwrites & out-of-range reads
    {
        FileManager::SingleFile sf(filename);
        std::vector<uint8_t> base_data = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
        sf.writeFile(base_data, 0);
        assert(sf.fileSize() == 8);

        // Overwrite exactly the middle two bytes [3, 4] ('D', 'E') with 'X', 'Y'
        std::vector<uint8_t> patch = {'X', 'Y'};
        sf.writeFile(patch, 3);

        auto readback = sf.readFileBytes(0, 8);
        const uint8_t expected[] = {'A', 'B', 'C', 'X', 'Y', 'F', 'G', 'H'};
        assert(readback.size() == 8);
        assert(std::memcmp(readback.data(), expected, 8) == 0);

        // Read partially past EOF: read 10 bytes starting from offset 6 (only 2 bytes exist: 'G', 'H')
        auto oob_read = sf.readFileBytes(6, 10);
        assert(oob_read.size() <= 2 && "Read beyond EOF must clamp to remaining bytes or fail safely");
        if (oob_read.size() == 2) {
            assert(oob_read[0] == 'G' && oob_read[1] == 'H');
        }

        // Read completely past EOF
        auto fully_oob = sf.readFileBytes(100, 10);
        assert(fully_oob.empty());
    }

    // 3. Persistence across different SingleFile instances / handles
    {
        const std::string persist_msg = "PersistentDataCheck_123456789";
        std::vector<uint8_t> w_buf(persist_msg.begin(), persist_msg.end());

        // Scope A: Open, write, flush, destroy object
        {
            FileManager::SingleFile sf1(filename);
            sf1.truncate(0);
            sf1.writeFile(w_buf, 0);
            sf1.flush();
        }

        // Scope B: Reopen in a new object and verify contents from flash
        {
            FileManager::SingleFile sf2(filename);
            assert(sf2.fileSize() == w_buf.size());
            auto r_buf = sf2.readFileBytes(0, w_buf.size());
            assert(r_buf == w_buf && "Data failed to persist across class instances!");
            sf2.deleteFile();
        }
    }

    ESP_LOGI(TAG, ">>> STRESS TEST 4 PASSED: All boundary & persistence checks exact! <<<");
}

// ============================================================================
// STRESS TEST 5: Read/Write Speed Benchmark (Chunk Sweep)
// Measures read/write throughput (KB/s) and IOPS for 64B, 512B, 1KB, 4KB, 8KB.
// ============================================================================
static void stress_test_rw_speed_benchmark() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 5] Read/Write Throughput & Latency Benchmark");

    const std::string filename = "stress_speed_bench";
    const size_t TEST_TOTAL_BYTES = 128 * 1024; // 128 KB per run
    const size_t CHUNK_SIZES[] = {64, 512, 1024, 2048, 4096, 8192};

    ESP_LOGI(TAG, "| Chunk Size | Write Speed (KB/s) | Write IOPS | Read Speed (KB/s) | Read IOPS |");
    ESP_LOGI(TAG, "|------------|--------------------|------------|-------------------|-----------|");

    for (size_t chunk_sz : CHUNK_SIZES) {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        std::vector<uint8_t> buffer(chunk_sz, 0xA5);
        size_t total_ops = TEST_TOTAL_BYTES / chunk_sz;

        // 1. Write Benchmark
        int64_t t_w0 = esp_timer_get_time();
        for (size_t i = 0; i < total_ops; ++i) {
            sf.writeFile(buffer, -1);
            if (i % 64 == 0) vTaskDelay(1); // Keep watchdog calm
        }
        sf.flush();
        int64_t t_w1 = esp_timer_get_time();

        double write_sec = (double)(t_w1 - t_w0) / 1000000.0;
        double write_kb_s = ((double)TEST_TOTAL_BYTES / 1024.0) / write_sec;
        double write_iops = (double)total_ops / write_sec;

        // 2. Read Benchmark
        int64_t t_r0 = esp_timer_get_time();
        for (size_t i = 0; i < total_ops; ++i) {
            auto r = sf.readFileBytes(static_cast<long>(i * chunk_sz), static_cast<long>(chunk_sz));
            if (i % 64 == 0) vTaskDelay(1);
        }
        int64_t t_r1 = esp_timer_get_time();

        double read_sec = (double)(t_r1 - t_r0) / 1000000.0;
        double read_kb_s = ((double)TEST_TOTAL_BYTES / 1024.0) / read_sec;
        double read_iops = (double)total_ops / read_sec;

        ESP_LOGI(TAG, "| %6zu B   | %18.2f | %10.1f | %17.2f | %9.1f |",
                 chunk_sz, write_kb_s, write_iops, read_kb_s, read_iops);

        sf.deleteFile();
    }

    ESP_LOGI(TAG, ">>> STRESS TEST 5 PASSED: Benchmark matrix completed! <<<");
}

// ============================================================================
// STRESS TEST 6: Truncate Speed & Behavior Benchmark
// Measures latency of progressive file shrinking vs. expanding.
// ============================================================================
static void stress_test_truncate_performance() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 6] Truncate Speed & Latency (Shrink vs Grow)");

    const std::string filename = "stress_trunc_test";
    const size_t INITIAL_SIZE = 256 * 1024; // 256 KB
    const size_t STEP = 32 * 1024;          // 32 KB steps

    FileManager::SingleFile sf(filename);
    sf.truncate(0);

    // Populate initial file
    std::vector<uint8_t> dummy(4096, 0xEE);
    for (size_t written = 0; written < INITIAL_SIZE; written += dummy.size()) {
        sf.writeFile(dummy, -1);
    }
    assert(sf.fileSize() == INITIAL_SIZE);

    ESP_LOGI(TAG, "--- Measuring Shrink Latency (Step: -32 KB) ---");
    size_t curr_sz = INITIAL_SIZE;
    while (curr_sz > 0) {
        size_t target_sz = (curr_sz >= STEP) ? (curr_sz - STEP) : 0;

        int64_t t0 = esp_timer_get_time();
        sf.truncate(target_sz);
        int64_t elapsed_us = esp_timer_get_time() - t0;

        assert(sf.fileSize() == target_sz);
        ESP_LOGI(TAG, "Shrunk from %6zu KB -> %6zu KB in %6lld us", 
                 curr_sz / 1024, target_sz / 1024, elapsed_us);
        curr_sz = target_sz;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    ESP_LOGI(TAG, "--- Measuring Growth Latency (Sparse/Zero Truncate) ---");
    // Expand file without writing data
    const size_t EXPAND_SIZES[] = {16 * 1024, 64 * 1024, 128 * 1024, 256 * 1024};
    for (size_t new_sz : EXPAND_SIZES) {
        int64_t t0 = esp_timer_get_time();
        sf.truncate(new_sz);
        int64_t elapsed_us = esp_timer_get_time() - t0;

        assert(sf.fileSize() == new_sz);
        ESP_LOGI(TAG, "Expanded to %6zu KB in %6lld us", new_sz / 1024, elapsed_us);
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    sf.deleteFile();
    ESP_LOGI(TAG, ">>> STRESS TEST 6 PASSED: Truncation behavior fully verified! <<<");
}
// ============================================================================
// STRESS TEST 8: shiftBytes Deterministic Correctness & Boundary Edge Cases
// Specifically targets overlap direction, 1-byte shifts, and hole insertions.
// ============================================================================
static void stress_test_shift_bytes_correctness() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 8] shiftBytes Deterministic Correctness Tests");

    const std::string filename = "stress_shift_det";

    // ------------------------------------------------------------------------
    // CASE 1: Right-Shift Overlap Test (The "memmove" Backwards-Copy Bug Test)
    // Range = 1024 bytes, ShiftDistance = 100 bytes (Distance < Range).
    // If shiftBytes copies forwards instead of backwards, data will be corrupted!
    // ------------------------------------------------------------------------
    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        const size_t DATA_LEN = 1024;
        const size_t SHIFT_DIST = 100;
        std::vector<uint8_t> init_data(DATA_LEN);
        for (size_t i = 0; i < DATA_LEN; ++i) init_data[i] = static_cast<uint8_t>(i & 0xFF);

        sf.writeFile(init_data, 0);

        // Shift entire 1024 bytes right by 100 bytes: [0, 1024) -> [100, 1124)
        sf.shiftBytes(SHIFT_DIST, /*left=*/false, 0, DATA_LEN);

        // Verify the shifted range [100, 1124)
        auto shifted = sf.readFileBytes(SHIFT_DIST, DATA_LEN);
        assert(shifted.size() == DATA_LEN);
        for (size_t i = 0; i < DATA_LEN; ++i) {
            if (shifted[i] != init_data[i]) {
                ESP_LOGE(TAG, "Right-shift overlap corruption at idx %zu! (Got 0x%02X, expected 0x%02X)",
                         i, shifted[i], init_data[i]);
                assert(false && "shiftBytes right-shift forward-copy bug detected!");
            }
        }
        ESP_LOGI(TAG, "-> Case 1 PASSED: Right shift overlapping range handled cleanly.");
        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // CASE 2: Middle Gap Insertion (Right Shift Sub-Region)
    // File: [AAAA][BBBB] -> Shift [BBBB] right by 4 -> [AAAA][....][BBBB]
    // ------------------------------------------------------------------------
    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        std::vector<uint8_t> partA(512, 'A');
        std::vector<uint8_t> partB(512, 'B');
        sf.writeFile(partA, -1);
        sf.writeFile(partB, -1);
        assert(sf.fileSize() == 1024);

        const size_t GAP = 256;
        // Shift partB right by GAP: range [512, 1024) moves to [768, 1280)
        sf.shiftBytes(GAP, /*left=*/false, 512, 1024);

        // 1. Verify partA remained untouched
        auto checkA = sf.readFileBytes(0, 512);
        assert(checkA == partA);

        // 2. Fill the gap with 'X'
        std::vector<uint8_t> partX(GAP, 'X');
        sf.writeFile(partX, 512);

        // 3. Verify partB at its new position [768, 1280)
        auto checkB = sf.readFileBytes(512 + GAP, 512);
        assert(checkB == partB);

        ESP_LOGI(TAG, "-> Case 2 PASSED: Middle gap insertion (splice) verified.");
        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // CASE 3: Middle Compaction (Left Shift Sub-Region)
    // File: [AAAA][XXXX][BBBB] -> Shift [BBBB] left to overwrite [XXXX]
    // ------------------------------------------------------------------------
    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        std::vector<uint8_t> partA(512, 'A');
        std::vector<uint8_t> partX(256, 'X');
        std::vector<uint8_t> partB(512, 'B');
        sf.writeFile(partA, -1);
        sf.writeFile(partX, -1);
        sf.writeFile(partB, -1);
        assert(sf.fileSize() == 1280);

        // Shift partB from [768, 1280) left by 256 -> moves to [512, 1024)
        sf.shiftBytes(256, /*left=*/true, 768, 1280);
        sf.truncate(1024);

        auto checkA = sf.readFileBytes(0, 512);
        auto checkB = sf.readFileBytes(512, 512);
        assert(checkA == partA && "Left shift corrupted pre-boundary data!");
        assert(checkB == partB && "Left shift corrupted shifted data!");

        ESP_LOGI(TAG, "-> Case 3 PASSED: Middle compaction verified.");
        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // CASE 4: Odd / Unaligned 1-Byte & 13-Byte Shifts
    // Catches off-by-one errors on non-block-aligned distances.
    // ------------------------------------------------------------------------
    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        const size_t TOTAL = 3000; // Not a power of 2
        std::vector<uint8_t> pattern(TOTAL);
        for (size_t i = 0; i < TOTAL; ++i) pattern[i] = static_cast<uint8_t>((i * 17) & 0xFF);
        sf.writeFile(pattern, 0);

        // Shift left by 1 byte: [1, TOTAL) -> [0, TOTAL - 1)
        sf.shiftBytes(1, /*left=*/true, 1, TOTAL);
        sf.truncate(TOTAL - 1);

        auto r1 = sf.readFileBytes(0, TOTAL - 1);
        assert(r1.size() == TOTAL - 1);
        assert(std::memcmp(r1.data(), pattern.data() + 1, TOTAL - 1) == 0 && "1-byte shift left failed!");

        // Shift left by 13 bytes on the remaining file
        sf.shiftBytes(13, /*left=*/true, 13, TOTAL - 1);
        sf.truncate(TOTAL - 1 - 13);

        auto r2 = sf.readFileBytes(0, TOTAL - 1 - 13);
        assert(std::memcmp(r2.data(), pattern.data() + 14, TOTAL - 1 - 13) == 0 && "13-byte shift left failed!");

        ESP_LOGI(TAG, "-> Case 4 PASSED: Unaligned 1-byte & 13-byte shifts 100%% exact.");
        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // CASE 5: Zero Distance / Empty Range No-Op Check
    // ------------------------------------------------------------------------
    {
        FileManager::SingleFile sf(filename);
        sf.truncate(0);
        std::vector<uint8_t> dummy = {1, 2, 3, 4, 5};
        sf.writeFile(dummy, 0);

        // Shift distance 0
        sf.shiftBytes(0, /*left=*/true, 0, 5);
        // Empty range
        sf.shiftBytes(3, /*left=*/false, 2, 2);

        auto check = sf.readFileBytes(0, 5);
        assert(check == dummy && "Zero-shift or empty range modified the file!");

        sf.deleteFile();
        ESP_LOGI(TAG, "-> Case 5 PASSED: Zero-distance & empty range no-op verified.");
    }

    ESP_LOGI(TAG, ">>> STRESS TEST 8 PASSED: shiftBytes core correctness verified! <<<");
}

 
// ============================================================================
// STRESS TEST 7: Sustained Big Data Streaming & Misaligned Read Verification
// Writes 512 KB to 1 MB using PRNG and reads back with mismatched chunk sizes.
// ============================================================================
static void stress_test_big_data_streaming() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[STRESS 7] Big Data Streaming (512 KB) & Misaligned Block Checks");

    const std::string filename = "stress_big_data";
    const size_t TOTAL_SIZE = 512 * 1024; // 512 KB (Increase to 1024 KB if your partition allows)
    const size_t WRITE_CHUNK = 4096;      // 4 KB write boundary
    const size_t READ_CHUNK = 3072;       // 3 KB read boundary (intentionally mismatched!)

    // Fast seedable PRNG (Linear Congruential Generator) to verify bytes on the fly without RAM
    auto prng = [](uint32_t state) -> uint8_t {
        return static_cast<uint8_t>((state * 1103515245u + 12345u) >> 16);
    };

    FileManager::SingleFile sf(filename);
    sf.truncate(0);

    // 1. Streaming Big Data Write
    ESP_LOGI(TAG, "Streaming %zu KB to flash in %zu-byte chunks...", TOTAL_SIZE / 1024, WRITE_CHUNK);
    int64_t t_w0 = esp_timer_get_time();

    std::vector<uint8_t> w_chunk(WRITE_CHUNK);
    uint32_t state_w = 42;
    for (size_t offset = 0; offset < TOTAL_SIZE; offset += WRITE_CHUNK) {
        for (size_t i = 0; i < WRITE_CHUNK; ++i) {
            w_chunk[i] = prng(state_w++);
        }
        bool ok = sf.writeFile(w_chunk, -1);
        assert(ok);

        if ((offset / WRITE_CHUNK) % 16 == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    sf.flush();
    int64_t t_w1 = esp_timer_get_time();

    double write_sec = (double)(t_w1 - t_w0) / 1000000.0;
    ESP_LOGW(TAG, "Big File Written: %.2f KB/s (Total time: %lld ms)", 
             ((double)TOTAL_SIZE / 1024.0) / write_sec, (t_w1 - t_w0) / 1000);

    // 2. Misaligned Streaming Read & Byte-by-Byte Verification
    ESP_LOGI(TAG, "Verifying data using mismatched chunk size (%zu bytes)...", READ_CHUNK);
    int64_t t_r0 = esp_timer_get_time();

    uint32_t state_r = 42;
    size_t verified_bytes = 0;
    while (verified_bytes < TOTAL_SIZE) {
        size_t to_read = std::min(READ_CHUNK, TOTAL_SIZE - verified_bytes);
        auto r_data = sf.readFileBytes(static_cast<long>(verified_bytes), static_cast<long>(to_read));
        assert(r_data.size() == to_read && "Read truncated unexpectedly");

        for (size_t i = 0; i < to_read; ++i) {
            uint8_t expected = prng(state_r++);
            if (r_data[i] != expected) {
                ESP_LOGE(TAG, "Big data corruption at byte offset %zu! (Got 0x%02X, Expected 0x%02X)",
                         verified_bytes + i, r_data[i], expected);
                assert(false && "Flash data diverged under sustained streaming!");
            }
        }
        verified_bytes += to_read;

        if ((verified_bytes / READ_CHUNK) % 20 == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    int64_t t_r1 = esp_timer_get_time();

    double read_sec = (double)(t_r1 - t_r0) / 1000000.0;
    ESP_LOGW(TAG, "Big File Verified: %.2f KB/s (Total time: %lld ms)", 
             ((double)TOTAL_SIZE / 1024.0) / read_sec, (t_r1 - t_r0) / 1000);

    sf.deleteFile();
    ESP_LOGI(TAG, ">>> STRESS TEST 7 PASSED: Big data verified without corruption or leaks! <<<");
}


 // ============================================================================
// STRESS TEST: shiftBytes Contract & Correctness Verification (Temp-File Version)
// Validates underflow guard, automatic compaction, deterministic zeroed holes,
// multi-sector streaming, and unaligned sub-region splicing.
// ============================================================================
static void test_shift_bytes_exact_contract() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST] Verifying shiftBytes Contract & Sub-Region Logic");

    const std::string filename = "shift_contract_test";

    // ------------------------------------------------------------------------
    // SUB-TEST 1: Underflow Guard & Default Arguments on Left Shift
    // ------------------------------------------------------------------------
    {
        ESP_LOGI(TAG, "-> Sub-test 1: Underflow guard check...");
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        std::vector<uint8_t> data = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        sf.writeFile(data, 0);

        // A. Default startOffset (-1 -> 0): 0 < 3 -> Underflow guard must reject without corrupting
        sf.shiftBytes(3, /*left=*/true); // startOffset defaults to 0
        assert(sf.fileSize() == 10 && "Underflow rejected operation must not alter file size");
        auto r1 = sf.readFileBytes(0, 10);
        assert(r1 == data && "Underflow guard failed to reject default startOffset on left shift!");

        // B. Explicit invalid start: startOffset (2) < shiftDistance (5) -> must reject
        sf.shiftBytes(5, /*left=*/true, 2, 10);
        assert(sf.fileSize() == 10);
        auto r2 = sf.readFileBytes(0, 10);
        assert(r2 == data && "Underflow guard failed on startOffset < shiftDistance!");

        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // SUB-TEST 2: Left Shift (Automatic Compaction & Exact Sizing)
    // File: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9] (10 bytes)
    // Shift range [3, 10) left by 3 -> Source moves to index [0, 7)
    // ------------------------------------------------------------------------
    {
        ESP_LOGI(TAG, "-> Sub-test 2: Left shift automatic compaction...");
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        std::vector<uint8_t> data = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        sf.writeFile(data, 0);

        // Move source [3, 10) left by 3
        sf.shiftBytes(3, /*left=*/true, 3, 10);

        // The temp-file implementation automatically shrinks the file:
        // Size MUST be exactly 7 (no truncate() needed and no trailing duplicate garbage!)
        assert(sf.fileSize() == 7 && "Temp-file shiftBytes must automatically compact the file size");

        auto compacted = sf.readFileBytes(0, 7);
        const uint8_t expected_final[] = {'3', '4', '5', '6', '7', '8', '9'};
        assert(compacted.size() == 7);
        assert(std::memcmp(compacted.data(), expected_final, 7) == 0 && "Compacted contents diverged");

        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // SUB-TEST 3: Middle Sub-Region Compaction (Splicing Out a Chunk)
    // File: [AAAA][XXXX][BBBB] -> 4 bytes 'A', 2 bytes 'X', 4 bytes 'B' (10 bytes)
    // Shift range [6, 10) ("BBBB") left by 2 to splice out "XXXX"
    // ------------------------------------------------------------------------
    {
        ESP_LOGI(TAG, "-> Sub-test 3: Middle sub-region splice (cut out middle chunk)...");
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        std::string raw = "AAAAXXBBBB";
        std::vector<uint8_t> data(raw.begin(), raw.end());
        sf.writeFile(data, 0);
        assert(sf.fileSize() == 10);

        // Shift [6, 10) ("BBBB") left by 2 -> target start is 6 - 2 = 4 (overwrites "XX")
        sf.shiftBytes(2, /*left=*/true, 6, 10);

        // Resulting file must be 8 bytes: "AAAABBBB"
        assert(sf.fileSize() == 8 && "Splicing middle chunk must shrink file by shiftDistance");
        auto spliced = sf.readFileBytes(0, 8);
        std::string spliced_str(spliced.begin(), spliced.end());
        assert(spliced_str == "AAAABBBB" && "Sub-region splice failed to preserve prefix and suffix!");

        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // SUB-TEST 4: Right Shift (Expansion + Zeroed Gap Verification)
    // File: [H, E, L, L, O, W, O, R, L, D] (10 bytes)
    // Shift range [5, 10) ("WORLD") right by 5 -> moves to [10, 15)
    // ------------------------------------------------------------------------
    {
        ESP_LOGI(TAG, "-> Sub-test 4: Right shift hole creation & zero-fill check...");
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        std::string initial_str = "HELLOWORLD";
        std::vector<uint8_t> data(initial_str.begin(), initial_str.end());
        sf.writeFile(data, 0);

        // Shift [5, 10) right by 5
        sf.shiftBytes(5, /*left=*/false, 5, 10);

        // File size must expand from 10 to 15
        assert(sf.fileSize() == 15);

        // Verify the 5-byte created hole at [5, 10) is cleanly zero-filled (0x00)
        auto hole = sf.readFileBytes(5, 5);
        assert(hole.size() == 5);
        for (size_t i = 0; i < 5; ++i) {
            assert(hole[i] == 0x00 && "Right-shift gap was not properly zero-initialized!");
        }

        // Verify the shifted suffix "WORLD" at [10, 15)
        auto suffix = sf.readFileBytes(10, 5);
        std::string suffix_str(suffix.begin(), suffix.end());
        assert(suffix_str == "WORLD" && "Right-shifted suffix data corrupted!");

        // Overwrite the hole with "_SPC_"
        std::string hole_data = "_SPC_";
        std::vector<uint8_t> h_buf(hole_data.begin(), hole_data.end());
        sf.writeFile(h_buf, 5);

        // Verify full file: "HELLO_SPC_WORLD"
        auto full_buf = sf.readFileBytes(0, 15);
        std::string full_str(full_buf.begin(), full_buf.end());
        assert(full_str == "HELLO_SPC_WORLD");

        sf.deleteFile();
    }

    // ------------------------------------------------------------------------
    // SUB-TEST 5: Multi-Sector (16 KB) Stream & Unaligned Chunk Verification
    // Tests streaming across multiple 4096-byte flash boundaries with odd sizes.
    // ------------------------------------------------------------------------
    {
        ESP_LOGI(TAG, "-> Sub-test 5: Multi-sector 16 KB unaligned streaming...");
        FileManager::SingleFile sf(filename);
        sf.truncate(0);

        const size_t TOTAL_SIZE = 16384; // 4 flash sectors
        const size_t DIST = 127;         // Prime, odd-byte shift distance
        std::vector<uint8_t> pattern(TOTAL_SIZE);
        for (size_t i = 0; i < TOTAL_SIZE; ++i) {
            pattern[i] = static_cast<uint8_t>((i * 13 + 3) & 0xFF);
        }
        sf.writeFile(pattern, 0);

        // Shift left: [127, 16384) left by 127 -> destination is [0, 16257)
        sf.shiftBytes(DIST, /*left=*/true, DIST, TOTAL_SIZE);

        assert(sf.fileSize() == TOTAL_SIZE - DIST);

        auto verified = sf.readFileBytes(0, TOTAL_SIZE - DIST);
        assert(verified.size() == TOTAL_SIZE - DIST);
        assert(std::memcmp(verified.data(), pattern.data() + DIST, TOTAL_SIZE - DIST) == 0 &&
               "Multi-sector unaligned streaming diverged!");

        sf.deleteFile();
    }

    ESP_LOGI(TAG, ">>> ALL shiftBytes CONTRACT TESTS PASSED PERFECTLY! <<<");
}

// ============================================================================
// STRESS TEST: shiftBytes Throughput & Latency Profile (Temp-File Version)
// Evaluates sequential throughput (KB/s) and latency using the new implementation.
// ============================================================================
static void test_shift_bytes_performance_profile() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[BENCHMARK] Profiling shiftBytes I/O Latency");

    const std::string filename = "shift_perf_profile";
    const size_t TEST_FILE_SIZE = 128 * 1024; // 128 KB
    const size_t SHIFT_SIZES[] = {64, 512, 2048, 4096, 16384};

    ESP_LOGI(TAG, "| Shift Distance | Direction | Time (ms) | Effective Speed (KB/s) |");
    ESP_LOGI(TAG, "|----------------|-----------|-----------|------------------------|");

    for (size_t dist : SHIFT_SIZES) {
        // --------------------------------------------------------------------
        // 1. Benchmark Left Shift (Pop / Compaction)
        // --------------------------------------------------------------------
        {
            FileManager::SingleFile sf(filename);
            sf.truncate(0);

            // Populate fresh 128 KB file
            std::vector<uint8_t> pattern(4096, 0x3C);
            for (size_t w = 0; w < TEST_FILE_SIZE; w += pattern.size()) {
                sf.writeFile(pattern, -1);
            }
            sf.flush();

            size_t target_moved_bytes = TEST_FILE_SIZE - dist;
            int64_t t_l0 = esp_timer_get_time();
            sf.shiftBytes(dist, /*left=*/true, dist, TEST_FILE_SIZE);
            int64_t t_l1 = esp_timer_get_time();

            assert(sf.fileSize() == target_moved_bytes);

            int64_t l_us = t_l1 - t_l0;
            double l_speed = ((double)target_moved_bytes / 1024.0) / ((double)l_us / 1000000.0);
            ESP_LOGI(TAG, "| %8zu B      |   LEFT    | %9.1f | %22.2f |", 
                     dist, (double)l_us / 1000.0, l_speed);

            sf.deleteFile();
        }

        vTaskDelay(pdMS_TO_TICKS(10));

        // --------------------------------------------------------------------
        // 2. Benchmark Right Shift (Insertion gap creation)
        // --------------------------------------------------------------------
        {
            FileManager::SingleFile sf(filename);
            sf.truncate(0);

            // Populate file of size (TEST_FILE_SIZE - dist)
            size_t initial_bytes = TEST_FILE_SIZE - dist;
            std::vector<uint8_t> pattern(4096, 0x3C);
            for (size_t w = 0; w < initial_bytes; w += pattern.size()) {
                size_t chunk = std::min(pattern.size(), initial_bytes - w);
                sf.writeFile(std::vector<uint8_t>(pattern.begin(), pattern.begin() + chunk), -1);
            }
            sf.flush();

            int64_t t_r0 = esp_timer_get_time();
            sf.shiftBytes(dist, /*left=*/false, 0, initial_bytes);
            int64_t t_r1 = esp_timer_get_time();

            assert(sf.fileSize() == TEST_FILE_SIZE);

            int64_t r_us = t_r1 - t_r0;
            double r_speed = ((double)initial_bytes / 1024.0) / ((double)r_us / 1000000.0);
            ESP_LOGI(TAG, "| %8zu B      |   RIGHT   | %9.1f | %22.2f |", 
                     dist, (double)r_us / 1000.0, r_speed);

            sf.deleteFile();
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}









// ============================================================================
// FreeRTOS Task Runner with Heap Leak Detection
// ============================================================================
extern "C" void run_singlefile_stress_suite(void* pvParameters) {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "Starting Heavy-Lift Stress Suite in 1 second...");
    ESP_LOGI(TAG, "Initial Free Heap: %u bytes", (unsigned)esp_get_free_heap_size());
    vTaskDelay(pdMS_TO_TICKS(1000));

    size_t start_heap = esp_get_free_heap_size();

    // Original Tests
    // stress_test_fifo_queue_compaction();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // stress_test_random_access_shadow_mirror();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // stress_test_large_file_streaming();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // // New Benchmarks & Edge-Case Tests
    // stress_test_correctness_and_boundaries();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // stress_test_rw_speed_benchmark();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // stress_test_truncate_performance();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // stress_test_big_data_streaming();
    // vTaskDelay(pdMS_TO_TICKS(100));




   test_shift_bytes_exact_contract();
    vTaskDelay(pdMS_TO_TICKS(50));

   

    test_shift_bytes_performance_profile();
    vTaskDelay(pdMS_TO_TICKS(50));











    size_t end_heap = esp_get_free_heap_size();
    size_t min_heap = esp_get_minimum_free_heap_size();

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "Heavy-Lift Suite COMPLETE!");
    ESP_LOGW(TAG, "Heap Summary -> Start: %u B | End: %u B | Delta: %d B | Lowest Ever: %u B",
             (unsigned)start_heap, (unsigned)end_heap, (int)(end_heap - start_heap), (unsigned)min_heap);
    ESP_LOGI(TAG, "==================================================");

   // assert((start_heap - end_heap) < 512 && "Memory leak detected across stress tests!");

    vTaskDelete(NULL);
}

extern "C" void app_main() {
    FileManager::init();
    // Allocate 12 KB task stack for the stress runner
    xTaskCreate(run_singlefile_stress_suite, "sf_stress", 12288, nullptr, 5, nullptr);
}
#endif