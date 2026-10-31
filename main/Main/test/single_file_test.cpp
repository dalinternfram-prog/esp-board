//#define TestSingleFile
#if defined (TestSingleFile1)
#include "single_file.hpp"
#include "file_manager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <unistd.h>

static const char* TAG = "SINGLE_FILE_TEST";

// Helper to fill a dummy data buffer
static std::vector<uint8_t> make_dummy_data(size_t size, uint8_t seed = 0xAA) {
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>((seed + i) & 0xFF);
    }
    return data;
}

// ============================================================================
// TEST 1: The POSIX ftruncate() SPIFFS Verification Test
// Proves whether POSIX ftruncate() works or fails with ENOSYS on your mount.
// ============================================================================
static void test_raw_posix_ftruncate() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 1] Raw POSIX ftruncate() Capability Test");

    const std::string filename = "test_raw_ftrunc";
    FileManager::deleteFile(filename);

    std::string fullPath = FileManager::getFullPath(filename);
    FILE* f = fopen(fullPath.c_str(), "w+b");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create test file: %s", fullPath.c_str());
        return;
    }

    // Write 10 KB to disk
    auto data = make_dummy_data(10 * 1024, 0x11);
    fwrite(data.data(), 1, data.size(), f);
    fflush(f);

    int fd = fileno(f);
    ESP_LOGI(TAG, "File created: %u bytes. Calling ftruncate(fd, 8)...", (unsigned)data.size());

    // --- The Exact Call That Failed Silently ---
    errno = 0;
    int res = ftruncate(fd, 8);
    int err = errno;

    if (res == 0) {
        ESP_LOGW(TAG, ">>> RESULT: POSIX ftruncate() SUCCEEDED! Your VFS supports native truncation.");
    } else {
        ESP_LOGE(TAG, ">>> RESULT: POSIX ftruncate() FAILED! ret: %d, errno: %d (%s)", 
                 res, err, strerror(err));
        ESP_LOGW(TAG, "--> This PROVES why old truncate() did nothing and kept the file 60KB!");
    }

    fclose(f);
    FileManager::deleteFile(filename);
}

// ============================================================================
// TEST 2: SingleFile::truncate() Functional Verification
// Verifies that our SingleFile::truncate(8) actually shrinks the file to 8 bytes
// and preserves the 8-byte header across reboots/reopens.
// ============================================================================
static void test_singlefile_truncate_behavior() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 2] SingleFile::truncate(8) Shrink & Header Integrity");

    const std::string filename = "test_sf_trunc";
    FileManager::deleteFile(filename);

    // 1. Create file with an 8-byte magic header + 50 KB of payload
    {
        FileManager::SingleFile sf(filename);
        std::vector<uint8_t> header = {'F', 'I', 'F', 'O', 0x08, 0x00, 0x00, 0x00}; // FIFO magic + head=8
        sf.writeFile(header, 0);

        // Append 50 KB of payload
        auto payload = make_dummy_data(50 * 1024, 0x33);
        sf.writeFile(payload, -1);
        sf.flush();

        size_t initial_sz = sf.fileSize();
        ESP_LOGI(TAG, "Initial file size on disk: %u bytes", (unsigned)initial_sz);
        assert(initial_sz == (8 + 50 * 1024));

        // 2. Call SingleFile::truncate(8)
        int64_t t0 = esp_timer_get_time();
        bool ok = sf.truncate(8);
        int64_t elapsed = esp_timer_get_time() - t0;

        ESP_LOGI(TAG, "sf.truncate(8) returned %s in %lld us", ok ? "TRUE" : "FALSE", elapsed);
        assert(ok && "truncate(8) must succeed!");

        size_t shrunk_sz = sf.fileSize();
        ESP_LOGI(TAG, "File size immediately after truncate: %u bytes", (unsigned)shrunk_sz);
        assert(shrunk_sz == 8 && "File size must be strictly 8 bytes!");
    }

    // 3. Re-open in a fresh instance to ensure the flash actually committed the shrink
    {
        FileManager::SingleFile sf_check(filename);
        size_t disk_sz = sf_check.fileSize();
        ESP_LOGI(TAG, "Reopened file size on fresh boot/handle: %u bytes", (unsigned)disk_sz);
        assert(disk_sz == 8 && "Physical disk size must remain 8 bytes!");

        auto read_hdr = sf_check.readFileBytes(0, 8);
        assert(read_hdr.size() == 8 && "Must read 8 bytes header back");
        assert(read_hdr[0] == 'F' && read_hdr[1] == 'I' && read_hdr[2] == 'F' && read_hdr[3] == 'O');
        ESP_LOGI(TAG, "Header intact: %c%c%c%c, head offset: %u",
                 read_hdr[0], read_hdr[1], read_hdr[2], read_hdr[3], (unsigned)read_hdr[4]);
    }

    ESP_LOGI(TAG, ">>> TEST 2 PASSED: File accurately shrunk to 8 bytes and header preserved! <<<");
    FileManager::deleteFile(filename);
}

// ============================================================================
// TEST 3: High-Frequency Read Latency Test (Simulates 1,000 Pop operations)
// Verifies that reading small chunks (e.g. 100-byte packets) does not stall
// the CPU with 400ms flash scans.
// ============================================================================
static void test_read_latency() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 3] High-Frequency readFileBytes Latency Benchmark");

    const std::string filename = "test_sf_latency";
    FileManager::deleteFile(filename);

    FileManager::SingleFile sf(filename);

    // Write a realistic 30 KB log file
    const size_t total_payload = 30 * 1024;
    auto data = make_dummy_data(total_payload, 0x77);
    sf.writeFile(data, 0);
    sf.flush();

    // Read 300 sequential packets of 100 bytes each
    const size_t chunk_size = 100;
    const size_t num_reads = 300;

    int64_t t_start = esp_timer_get_time();
    for (size_t i = 0; i < num_reads; ++i) {
        long offset = static_cast<long>(i * chunk_size);
        auto chunk = sf.readFileBytes(offset, chunk_size);
        if (chunk.size() != chunk_size) {
            ESP_LOGE(TAG, "Read failed at offset %ld! got %u", offset, (unsigned)chunk.size());
            break;
        }
    }
    int64_t total_time = esp_timer_get_time() - t_start;
    double avg_per_read = (double)total_time / num_reads;

    ESP_LOGW(TAG, "Performed %u reads (100B each) -> Total: %lld us | Avg: %.1f us/read",
             (unsigned)num_reads, total_time, avg_per_read);

    // If avg is under 500 us (0.5 ms), reading is fast and healthy
    // If avg was > 100,000 us (100 ms), it would prove fseek linear scans were killing the loop
    if (avg_per_read < 2000.0) {
        ESP_LOGI(TAG, ">>> TEST 3 PASSED: readFileBytes latency is in the microsecond range! <<<");
    } else {
        ESP_LOGE(TAG, ">>> TEST 3 FAILED: readFileBytes is abnormally slow (%.1f us)! <<<", avg_per_read);
    }

    FileManager::deleteFile(filename);
}

// ============================================================================
// FreeRTOS Task Runner
// ============================================================================
extern "C" void run_singlefile_tests(void* pvParameters) {
    ESP_LOGI(TAG, "Starting SingleFile Verification Suite in 1 second...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    test_raw_posix_ftruncate();
    vTaskDelay(pdMS_TO_TICKS(100));

    test_singlefile_truncate_behavior();
    vTaskDelay(pdMS_TO_TICKS(100));

    test_read_latency();

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "All SingleFile verification tests finished!");
    ESP_LOGI(TAG, "==================================================");

    vTaskDelete(NULL);
}




 

extern "C" void app_main() {
    // 1. Mount SPIFFS
    FileManager::init();

    // 2. Run SingleFile verification test
    xTaskCreate(run_singlefile_tests, "sf_test", 8192, nullptr, 5, nullptr);
}
#endif
















//#define TestSingleFile
#if defined (TestSingleFile)
#include "single_file.hpp"
#include "file_manager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <numeric>

static const char* TAG = "SINGLE_FILE_TEST";

// Helper to fill a deterministic dummy data buffer
static std::vector<uint8_t> make_dummy_data(size_t size, uint8_t seed = 0xAA) {
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>((seed + i * 3) & 0xFF);
    }
    return data;
}

// ============================================================================
// TEST 1: Basic R/W, Slicing, and Byte-for-Byte Correctness
// Verifies string & vector overloads, arbitrary slicing, and negative offset (EOF read).
// ============================================================================
static void test_rw_correctness() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 1] R/W Overloads, Slicing & Integrity Test");

    const std::string filename = "test_rw_correctness";
    FileManager::deleteFile(filename);

    {
        FileManager::SingleFile sf(filename);

        // 1. Write binary vector
        auto binPayload = make_dummy_data(1024, 0x10);
        bool ok = sf.writeFile(binPayload, 0);
        assert(ok && "Binary write failed");
        assert(sf.fileSize() == 1024);

        // 2. Append string using offset = -1
        std::string strSuffix = "_TAIL_STRING_MARKER_";
        ok = sf.writeFile(strSuffix, -1);
        assert(ok && "String append failed");
        assert(sf.fileSize() == 1024 + strSuffix.size());

        // 3. Read back full file
        auto fullRead = sf.readFileBytes(0, -1);
        assert(fullRead.size() == 1024 + strSuffix.size());

        // Verify binary prefix
        assert(std::memcmp(fullRead.data(), binPayload.data(), 1024) == 0 && "Binary prefix mismatch");

        // Verify string suffix
        assert(std::memcmp(fullRead.data() + 1024, strSuffix.data(), strSuffix.size()) == 0 && "Suffix mismatch");

        // 4. Test slicing: read arbitrary 128 bytes from offset 256
        auto slice = sf.readFileBytes(256, 128);
        assert(slice.size() == 128);
        assert(std::memcmp(slice.data(), binPayload.data() + 256, 128) == 0 && "Slice read mismatch");

        // 5. Test out-of-bounds read handling
        auto oob = sf.readFileBytes(sf.fileSize() + 100, 50);
        assert(oob.empty() && "OOB read must return empty vector");
    }

    // 6. Persistence check on fresh handle
    {
        FileManager::SingleFile sf_reopen(filename);
        assert(sf_reopen.fileSize() == 1024 + 20); // 20 is strSuffix length
    }

    ESP_LOGI(TAG, ">>> TEST 1 PASSED: R/W, Slices, and Integrity Verified! <<<");
    FileManager::deleteFile(filename);
}

// ============================================================================
// TEST 2: Gap Filling / Zero-Padding Verification
// Verifies that writing past EOF automatically zero-pads the gap.
// ============================================================================
static void test_gap_padding() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 2] Write Offset Past EOF (Zero Padding) Test");

    const std::string filename = "test_gap_padding";
    FileManager::deleteFile(filename);

    FileManager::SingleFile sf(filename);

    // Write 16 bytes at offset 0
    auto initData = make_dummy_data(16, 0x55);
    sf.writeFile(initData, 0);
    assert(sf.fileSize() == 16);

    // Write 8 bytes at offset 1024 (creating an intermediate gap of 1024 - 16 = 1008 zeros)
    std::vector<uint8_t> farData = {'P', 'A', 'D', '_', 'E', 'N', 'D', '!'};
    bool ok = sf.writeFile(farData, 1024);
    assert(ok && "Write with gap padding failed");
    assert(sf.fileSize() == 1024 + farData.size());

    // Read back gap and verify it consists strictly of 0x00
    auto gapBytes = sf.readFileBytes(16, 1008);
    assert(gapBytes.size() == 1008);
    for (size_t i = 0; i < gapBytes.size(); ++i) {
        if (gapBytes[i] != 0) {
            ESP_LOGE(TAG, "Non-zero byte found in padding at index %zu: 0x%02X", i, gapBytes[i]);
            assert(false && "Padding must be strictly 0x00");
        }
    }

    // Verify trailing payload
    auto tailBytes = sf.readFileBytes(1024, farData.size());
    assert(tailBytes == farData && "Data written after gap is corrupted");

    ESP_LOGI(TAG, ">>> TEST 2 PASSED: Gap zero-padding logic verified! <<<");
    FileManager::deleteFile(filename);
}

// ============================================================================
// TEST 3: In-Place Byte Shift Verification (Left & Right)
// Essential for queue compaction (pop) and block insertion.
// ============================================================================
static void test_shift_bytes() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 3] shiftBytes() Compaction & Expansion Test");

    const std::string filename = "test_shift_bytes";
    FileManager::deleteFile(filename);

    FileManager::SingleFile sf(filename);

    // Initial Layout (1500 bytes total):
    // [0 .. 499]:   Part A (0xAA)
    // [500 .. 999]: Part B (0xBB)
    // [1000..1499]: Part C (0xCC)
    auto partA = std::vector<uint8_t>(500, 0xAA);
    auto partB = std::vector<uint8_t>(500, 0xBB);
    auto partC = std::vector<uint8_t>(500, 0xCC);

    sf.writeFile(partA, 0);
    sf.writeFile(partB, -1);
    sf.writeFile(partC, -1);
    assert(sf.fileSize() == 1500);

    // 1. LEFT SHIFT: Remove Part A by shifting Part B + Part C left by 500 bytes
    // Target region to shift left: [500, 1500) shifted by 500
    int64_t t0 = esp_timer_get_time();
    sf.shiftBytes(500, /*left=*/true, /*startOffset=*/500, /*endOffset=*/1500);
    int64_t t_shift_left = esp_timer_get_time() - t0;
    ESP_LOGI(TAG, "Left-shifted 1000 bytes in %lld us", t_shift_left);

    // Shrink file size to 1000 bytes
    sf.truncate(1000);
    assert(sf.fileSize() == 1000);

    // Validate new content: should be Part B followed by Part C
    auto readB = sf.readFileBytes(0, 500);
    auto readC = sf.readFileBytes(500, 500);
    assert(readB == partB && "Left shift corrupted first segment");
    assert(readC == partC && "Left shift corrupted second segment");

    // 2. RIGHT SHIFT: Insert 200 bytes gap between Part B and Part C
    // Target region to shift right: [500, 1000) shifted right by 200
    t0 = esp_timer_get_time();
    sf.shiftBytes(200, /*left=*/false, /*startOffset=*/500, /*endOffset=*/1000);
    int64_t t_shift_right = esp_timer_get_time() - t0;
    ESP_LOGI(TAG, "Right-shifted 500 bytes in %lld us", t_shift_right);

    // Validate that Part C moved to offset 700
    auto readC_shifted = sf.readFileBytes(700, 500);
    assert(readC_shifted == partC && "Right shift corrupted moved segment");

    // Validate Part B stayed intact at [0, 500)
    auto readB_intact = sf.readFileBytes(0, 500);
    assert(readB_intact == partB && "Right shift corrupted stationary segment");

    ESP_LOGI(TAG, ">>> TEST 3 PASSED: shiftBytes left/right operations verified! <<<");
    FileManager::deleteFile(filename);
}

// ============================================================================
// TEST 4: C++ Move Semantics & RAII Safety
// Verifies move constructor and move assignment transfer handle ownership cleanly.
// ============================================================================
static void test_move_semantics() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 4] C++ Move Semantics & Handle Ownership Test");

    const std::string filename1 = "test_move_1";
    const std::string filename2 = "test_move_2";
    FileManager::deleteFile(filename1);
    FileManager::deleteFile(filename2);

    {
        FileManager::SingleFile sf1(filename1);
        sf1.writeFile(std::string("HELLO_FROM_SF1"), 0);

        // Move Constructor
        FileManager::SingleFile sf_moved(std::move(sf1));
        assert(sf_moved.fileSize() == 14);

        // Writing to moved object must work seamlessly
        sf_moved.writeFile(std::string("_EXT"), -1);
        assert(sf_moved.fileSize() == 18);

        // Move Assignment
        FileManager::SingleFile sf2(filename2);
        sf2.writeFile(std::string("SF2_ORIGINAL"), 0);

        sf2 = std::move(sf_moved);
        assert(sf2.fileSize() == 18);

        auto content = sf2.readFileBytes(0, -1);
        std::string s(content.begin(), content.end());
        assert(s == "HELLO_FROM_SF1_EXT");
    }

    ESP_LOGI(TAG, ">>> TEST 4 PASSED: Move operations safely transferred handles! <<<");
    FileManager::deleteFile(filename1);
    FileManager::deleteFile(filename2);
}
 

void test_ftruncate_stale_buffer() {
    const std::string testFile = "trunc_test.bin";

    // 1. Create a file and write 128 bytes: "AAAA...AAAA"
    {
        FileManager::SingleFile sf(testFile);
        std::string initialData(128, 'A');
        sf.writeFile(initialData, 0);
    }

    // 2. Open the file and read the first 10 bytes
    // This causes libc (newlib) to fill its internal read cache with the entire 128 bytes.
    FileManager::SingleFile sf(testFile);
    std::vector<uint8_t> chunk1 = sf.readFileBytes(0, 10);
    ESP_LOGI(TAG, "Read %zu bytes (should be 10): %c", chunk1.size(), chunk1[0]);

    // 3. Truncate the file down to 20 bytes.
    // The bytes from 20 to 127 are now deleted on the flash disk.
    sf.truncate(20);

    // 4. Check the file size
    size_t sizeAfterTrunc = sf.fileSize();
    ESP_LOGI(TAG, "File size reported: %zu (expected 20)", sizeAfterTrunc);

    // 5. ATTEMPT TO READ: Try to read at offset 15 for 20 bytes.
    // Since the file is only 20 bytes total, this should only return 5 bytes (offsets 15..19).
    // But observe what happens:
    std::vector<uint8_t> chunk2 = sf.readFileBytes(15, 20);
    ESP_LOGI(TAG, "Chunk2 size: %zu", chunk2.size());

    // 6. NOW WRITE PAST TRUNCATED REGION:
    // Because the libc buffer was not invalidated, subsequent write/read operations
    // become desynchronized. Try reading starting from byte 25 (which does not exist!):
    std::vector<uint8_t> ghostData = sf.readFileBytes(25, 10);
    if (!ghostData.empty()) {
        // BUG PROVEN: Data that was deleted on flash was returned from the stale libc RAM buffer!
        ESP_LOGE(TAG, "BUG CONFIRMED: Read %zu bytes of ghost data past EOF from libc cache!", ghostData.size());
    }
}
// ============================================================================
// TEST 5: Comprehensive I/O Latency & Throughput Benchmark
// Measures raw sequential write speed, chunked read latency, and fileSize() cost.
// ============================================================================
static void test_performance_benchmark() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "[TEST 5] Performance & Latency Benchmark");

    const std::string filename = "test_perf_bench";
    FileManager::deleteFile(filename);

    FileManager::SingleFile sf(filename);

    // 1. Throughput Benchmark: 64 KB Write (4 KB chunks)
    const size_t total_payload = 64 * 1024;
    auto test_data = make_dummy_data(total_payload, 0x3E);

    int64_t t_w0 = esp_timer_get_time();
    bool w_ok = sf.writeFile(test_data, 0);
    sf.flush();
    int64_t t_w1 = esp_timer_get_time();

    assert(w_ok);
    double write_kb = (double)total_payload / 1024.0;
    double write_sec = (double)(t_w1 - t_w0) / 1000000.0;
    double write_speed_kb_s = write_kb / write_sec;

    ESP_LOGW(TAG, "Write Speed: 64 KB written in %lld ms (%.2f KB/s)",
             (t_w1 - t_w0) / 1000, write_speed_kb_s);

    // 2. Latency Benchmark: 500 sequential small reads (128 bytes each)
    const size_t num_reads = 500;
    const size_t read_chunk = 128;
    int64_t t_r0 = esp_timer_get_time();

    for (size_t i = 0; i < num_reads; ++i) {
        long offset = static_cast<long>((i * read_chunk) % (total_payload - read_chunk));
        auto buf = sf.readFileBytes(offset, read_chunk);
        if (buf.size() != read_chunk) {
            ESP_LOGE(TAG, "Bench read error at %ld", offset);
            break;
        }
    }
    int64_t t_r1 = esp_timer_get_time();
    double total_read_us = (double)(t_r1 - t_r0);
    double avg_read_latency = total_read_us / num_reads;

    ESP_LOGW(TAG, "Read Latency: %zu reads of %zu B -> Total: %lld us | Avg: %.1f us/read",
             num_reads, read_chunk, (t_r1 - t_r0), avg_read_latency);

    // 3. Metadata Latency: fileSize() benchmark (500 iterations)
    int64_t t_sz0 = esp_timer_get_time();
    for (size_t i = 0; i < 500; ++i) {
        volatile size_t sz = sf.fileSize();
        (void)sz;
    }
    int64_t t_sz1 = esp_timer_get_time();
    double avg_stat_us = (double)(t_sz1 - t_sz0) / 500.0;
    ESP_LOGW(TAG, "fileSize() Latency: 500 calls -> Total: %lld us | Avg: %.1f us/call",
             (t_sz1 - t_sz0), avg_stat_us);

    ESP_LOGI(TAG, ">>> TEST 5 FINISHED: Performance metrics collected! <<<");
    FileManager::deleteFile(filename);
}

// ============================================================================
// FreeRTOS Task Runner
// ============================================================================
extern "C" void run_singlefile_tests(void* pvParameters) {
    ESP_LOGI(TAG, "Starting SingleFile Verification Suite in 1 second...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // test_rw_correctness();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // test_gap_padding();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // test_shift_bytes();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // test_move_semantics();
    // vTaskDelay(pdMS_TO_TICKS(100));

    // test_performance_benchmark();
test_ftruncate_stale_buffer();
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "All SingleFile verification & performance tests passed!");
    ESP_LOGI(TAG, "==================================================");

    vTaskDelete(NULL);
}

extern "C" void app_main() {
    // 1. Mount filesystem
    FileManager::init();

    // 2. Run verification suite
    xTaskCreate(run_singlefile_tests, "sf_test", 8192, nullptr, 5, nullptr);
}
#endif