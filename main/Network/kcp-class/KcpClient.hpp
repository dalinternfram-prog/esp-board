// // // #pragma once
// // // #include "ikcp.h"
// // // #include <esp_log.h>
// // // #include <unistd.h>
// // // #include "../communicate-data/receive-send-net-data.hpp"
// // // namespace Network::ckpClient {
// // //     using namespace socket;
// // // class KcpAudioClient {
// // // private:
// // //     ikcpcb* kcp_ = nullptr;
// // //     UdpConnection conn_;
// // //     uint32_t conv_id_ = 0x20260315; // Example unique session ID

// // // TaskHandle_t kcp_task_handle_ = nullptr;
// // //     volatile bool running_ = false;
// // // /**
// // //  * @brief KCP output bridge callback.
// // //  * Whenever KCP wants to send a packet, it invokes this function.
// // //  */
// // // static int kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user) {
// // //     // 1. Recover our UdpConnection pointer from KCP's user field
// // //     auto* conn = static_cast<UdpConnection*>(user);
// // //     if (!conn || !conn->is_valid()) {
// // //         ESP_LOGE("KCP", "Output callback received invalid connection pointer!");
// // //         return -1;
// // //     }

// // //     // 2. Transmit the packet via our non-blocking UDP send helper
// // //     bool success = communicate::udp_send(*conn, reinterpret_cast<const uint8_t*>(buf), len);
    
// // //     // KCP expects 0 on success, or a negative error code on failure
// // //     return success ? 0 : -1;
// // // }
// // // static void kcp_task_trampoline(void* arg) {
// // //         auto* client = static_cast<KcpAudioClient*>(arg);
// // //         client->kcp_worker_loop();
// // //     }

// // // void kcp_worker_loop() {
// // //         uint8_t rx_buffer[1024];
// // //         struct sockaddr_storage src_addr;
// // //         socklen_t src_addr_len;

// // //         // Determine tick rate based on 10ms interval
// // //         const TickType_t xFrequency = pdMS_TO_TICKS(10);
// // //         TickType_t xLastWakeTime = xTaskGetTickCount();

// // //         ESP_LOGI("KCP", "KCP background worker task started.");

// // //         while (running_) {
// // //             // 1. Get current milliseconds clock for KCP
// // //             uint32_t current_ms = pdTICKS_TO_MS(xTaskGetTickCount());

// // //             // 2. Poll underlying UDP socket for incoming packets (Non-blocking)
// // //             src_addr_len = sizeof(src_addr);
// // //             ssize_t bytes_received = communicate::udp_receive(
// // //                 conn_.sock, 
// // //                 rx_buffer, 
// // //                 sizeof(rx_buffer), 
// // //                 &src_addr, 
// // //                 &src_addr_len
// // //             );

// // //             // 3. If data arrived, feed it directly into KCP's input parser
// // //             if (bytes_received > 0) {
// // //                 int err = ikcp_input(kcp_, reinterpret_cast<const char*>(rx_buffer), bytes_received);
// // //                 if (err < 0) {
// // //                     ESP_LOGW("KCP", "ikcp_input error parsing packet: %d", err);
// // //                 }
// // //             }

// // //             // 4. Drive KCP's internal clock and timers (Retransmissions, acknowledgments, etc.)
// // //             ikcp_update(kcp_, current_ms);

// // //             // 5. Periodic delay to maintain the 10ms loop cadence precisely
// // //             vTaskDelayUntil(&xLastWakeTime, xFrequency);
// // //         }

// // //         vTaskDelete(nullptr);
// // //     }

















// // // public:
// // //     bool initialize(const char* host, const char* port) {
// // //         // 1. Establish our underlying non-blocking UDP connection (from Task 2)
// // //         auto conn_opt = socket::setup_production_udp_socket(host, port);
// // //         if (!conn_opt || !conn_opt->is_valid()) {
// // //             ESP_LOGE("KCP", "Failed to initialize underlying UDP socket.");
// // //             return false;
// // //         }
// // //         conn_ = *conn_opt;

// // //         // 2. Create the KCP control block
// // //         // We pass our connection struct pointer into the 'user' parameter
// // //         kcp_ = ikcp_create(conv_id_, &conn_);
// // //         if (!kcp_) {
// // //             ESP_LOGE("KCP", "Failed to allocate KCP control block.");
// // //             close(conn_.sock);
// // //             return false;
// // //         }
// // // // 3. REGISTER THE OUTPUT CALLBACK HERE!
// // //         ikcp_setoutput(kcp_, kcp_output_callback);
// // //         ESP_LOGI("KCP", "KCP control block initialized successfully (conv: 0x%08X)", conv_id_);



// // // // 4. CONFIGURE KCP FOR LOW-LATENCY AUDIO ("Fast Mode")
// // //         // nodelay = 1, interval = 10ms, resend = 2 (fast retransmit), nc = 1 (disable congestion window)
// // //         ikcp_nodelay(kcp_, 1, 10, 2, 1);

// // //         // Optional: Set custom window sizes (e.g., 128 packets send/receive window)
// // //         ikcp_wndsize(kcp_, 128, 128);

// // //         ESP_LOGI("KCP", "KCP control block initialized, wired, and configured for low-latency audio!");
// // //         return true;









// // //         return true;
// // //     }

// // //     ~KcpAudioClient() {
// // //         if (kcp_) {
// // //             ikcp_release(kcp_); // Frees KCP internal buffers
// // //             kcp_ = nullptr;
// // //         }
// // //         if (conn_.is_valid()) {
// // //             close(conn_.sock); // Closes the underlying UDP socket
// // //         }
// // //     }

// // //     // Prevent copying to ensure socket/KCP safety
// // //     KcpAudioClient(const KcpAudioClient&) = delete;
// // //     KcpAudioClient& operator=(const KcpAudioClient&) = delete;
// // //     KcpAudioClient() = default;

// // //     ikcpcb* get_kcp() { return kcp_; }
// // //     const UdpConnection& get_connection() const { return conn_; }
// // // };
// // // } ;












// // #pragma once

// // #include <sys/socket.h>
// // #include <netdb.h>
// // #include <arpa/inet.h>
// // #include <optional>
// // #include <cstdint>
// // #include "ikcp.h"
// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"

// // // Structure to hold the socket handle and target destination data
// // struct UdpConnection {
// //     int sock = -1;
// //     struct sockaddr_storage addr = {};
// //     socklen_t addr_len = 0;

// //     bool is_valid() const { return sock >= 0; }
// // };

// // class KcpAudioClient {
// // private:
// //     ikcpcb* kcp_ = nullptr;
// //     UdpConnection conn_;
// //     uint32_t conv_id_ = 0x20260315;
    
// //     TaskHandle_t kcp_task_handle_ = nullptr;
// //     volatile bool running_ = false;

// //     // FreeRTOS background task hooks
// //     static void kcp_task_trampoline(void* arg);
// //     void kcp_worker_loop();

// //     // KCP bridge output callback
// //     static int kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user);

// // public:
// //     KcpAudioClient() = default;
// //     ~KcpAudioClient();

// //     // Prevent copying to ensure socket/KCP safety
// //     KcpAudioClient(const KcpAudioClient&) = delete;
// //     KcpAudioClient& operator=(const KcpAudioClient&) = delete;

// //     // Initialization & Lifecycle
// //     bool initialize(const char* host, const char* port);
// //     bool start_background_worker();
// //     void stop_background_worker();

// //     // Data Flow APIs
// //     int send_audio_data(const uint8_t* pcm_data, size_t size);
// //     int receive_audio_data(uint8_t* buffer, size_t max_size);

// //     // Getters
// //     ikcpcb* get_kcp() { return kcp_; }
// //     const UdpConnection& get_connection() const { return conn_; }
// // };



// #pragma once

// #include <sys/socket.h>
// #include <netdb.h>
// #include <optional>
// #include <cstdint>
// #include "ikcp.h"

// struct UdpConnection {
//     int sock = -1;
//     struct sockaddr_storage addr = {};
//     socklen_t addr_len = 0;

//     bool is_valid() const { return sock >= 0; }
// };

// class KcpAudioClient {
// private:
//     ikcpcb* kcp_ = nullptr;
//     UdpConnection conn_;
//     uint32_t conv_id_ = 0x20260315;

//     static int kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user);

// public:
//     KcpAudioClient() = default;
//     ~KcpAudioClient();

//     // Prevent copying
//     KcpAudioClient(const KcpAudioClient&) = delete;
//     KcpAudioClient& operator=(const KcpAudioClient&) = delete;

//     // Initialization & Cleanup
//     bool initialize(const char* host, const char* port);
//     void close_connection();

//     // Core Tick / Poll Methods (Called externally by your application loop/task)
//     void process_incoming();
//     void update(uint32_t current_ms);

//     // Data Flow APIs
//     int send_audio_data(const uint8_t* data, size_t size);
//     int receive_audio_data(uint8_t* buffer, size_t max_size);

//     // Getters
//     int get_socket() const { return conn_.sock; }
//     bool is_initialized() const { return kcp_ != nullptr && conn_.is_valid(); }
// };


















#pragma once

#include <sys/socket.h>
#include <netdb.h>
#include <optional>
#include <cstdint>
#include "ikcp.h"
#include <vector>
namespace Network {
    struct UdpConnection {
    int sock = -1;
    struct sockaddr_storage addr = {};
    socklen_t addr_len = 0;

    bool is_valid() const { return sock >= 0; }
};

class KcpClient {
private:
    ikcpcb* kcp_ = nullptr;
    UdpConnection conn_;
    
    protected:

    uint32_t conv_id_ ;

   // static int kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user);

public:
    KcpClient(uint32_t conv_id) :conv_id_(conv_id){};
    ~KcpClient();

    // Prevent copying
    KcpClient(const KcpClient&) = delete;
    KcpClient& operator=(const KcpClient&) = delete;

    // Initialization & Cleanup
    bool initialize(const char* host, const char* port);
    void close_connection();

    // Core Tick & Data Flow Methods
    void update(uint32_t current_ms);
 std::vector<uint8_t> poll_and_receive();
 //* you can safly free data;
    int send(const uint8_t* data, size_t size);

    // Getters
    int get_socket() const { return conn_.sock; }
    bool is_initialized() const { return kcp_ != nullptr && conn_.is_valid(); }
};
} ;
