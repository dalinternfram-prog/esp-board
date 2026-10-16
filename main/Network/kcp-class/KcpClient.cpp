// // #include "KcpAudioClient.hpp"
// // #include <fcntl.h>
// // #include <unistd.h>
// // #include <errno.h>
// // #include <esp_log.h>
// // #include <cstring>

// // static const char* TAG = "KCP_AUDIO";

// // // Helper: Set socket to non-blocking mode
// // static bool make_socket_non_blocking(int sock) {
// //     int flags = fcntl(sock, F_GETFL, 0);
// //     if (flags < 0) {
// //         ESP_LOGE(TAG, "Failed to get socket flags: %d", errno);
// //         return false;
// //     }
// //     if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
// //         ESP_LOGE(TAG, "Failed to set socket to non-blocking: %d", errno);
// //         return false;
// //     }
// //     return true;
// // }

// // // Helper: Production UDP socket setup with getaddrinfo (IPv4/IPv6 agnostic)
// // static std::optional<UdpConnection> setup_production_udp_socket(const char* host, const char* port) {
// //     struct addrinfo hints = {};
// //     hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6 dynamically
// //     hints.ai_socktype = SOCK_DGRAM;  // UDP datagram socket
// //     hints.ai_protocol = IPPROTO_UDP;

// //     struct addrinfo* res = nullptr;
// //     int err = getaddrinfo(host, port, &hints, &res);
    
// //     if (err != 0 || res == nullptr) {
// //         ESP_LOGE(TAG, "Failed to resolve hostname %s: error %d", host, err);
// //         return std::nullopt;
// //     }

// //     int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
// //     if (sock < 0) {
// //         ESP_LOGE(TAG, "Failed to create socket for family %d", res->ai_family);
// //         freeaddrinfo(res);
// //         return std::nullopt;
// //     }

// //     if (!make_socket_non_blocking(sock)) {
// //         close(sock);
// //         freeaddrinfo(res);
// //         return std::nullopt;
// //     }

// //     UdpConnection conn;
// //     conn.sock = sock;
// //     std::memcpy(&conn.addr, res->ai_addr, res->ai_addrlen);
// //     conn.addr_len = res->ai_addrlen;

// //     freeaddrinfo(res);
// //     ESP_LOGI(TAG, "UDP socket successfully created and set to non-blocking.");
// //     return conn;
// // }

// // // Helper: Non-blocking UDP send wrapper
// // static bool udp_send(const UdpConnection& conn, const uint8_t* data, size_t len) {
// //     if (!conn.is_valid()) {
// //         ESP_LOGE(TAG, "Attempted to send on an invalid socket!");
// //         return false;
// //     }

// //     ssize_t sent = sendto(conn.sock, data, len, 0, (struct sockaddr*)&conn.addr, conn.addr_len);
// //     if (sent < 0) {
// //         if (errno == EWOULDBLOCK || errno == EAGAIN) {
// //             ESP_LOGW(TAG, "Send buffer full (EWOULDBLOCK). Packet dropped.");
// //             return false;
// //         } else {
// //             ESP_LOGE(TAG, "sendto failed with error: %d", errno);
// //             return false;
// //         }
// //     }
// //     return true;
// // }

// // // Helper: Non-blocking UDP receive wrapper
// // static ssize_t udp_receive(int sock, uint8_t* buffer, size_t max_len, struct sockaddr_storage* src_addr, socklen_t* src_addr_len) {
// //     ssize_t received = recvfrom(sock, buffer, max_len, 0, (struct sockaddr*)src_addr, src_addr_len);
// //     if (received < 0) {
// //         if (errno == EWOULDBLOCK || errno == EAGAIN) {
// //             return 0; // No data available right now
// //         } else {
// //             ESP_LOGE(TAG, "recvfrom error: %d", errno);
// //             return -1;
// //         }
// //     }
// //     return received;
// // }

// // // KCP Output Bridge Callback
// // int KcpAudioClient::kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user) {
// //     auto* conn = static_cast<UdpConnection*>(user);
// //     if (!conn || !conn->is_valid()) {
// //         return -1;
// //     }
// //     bool success = udp_send(*conn, reinterpret_cast<const uint8_t*>(buf), len);
// //     return success ? 0 : -1;
// // }

// // KcpAudioClient::~KcpAudioClient() {
// //     stop_background_worker();
// //     if (kcp_) {
// //         ikcp_release(kcp_);
// //         kcp_ = nullptr;
// //     }
// //     if (conn_.is_valid()) {
// //         close(conn_.sock);
// //     }
// // }

// // bool KcpAudioClient::initialize(const char* host, const char* port) {
// //     // 1. Establish underlying transport
// //     auto conn_opt = setup_production_udp_socket(host, port);
// //     if (!conn_opt || !conn_opt->is_valid()) {
// //         ESP_LOGE(TAG, "Failed to initialize underlying UDP socket.");
// //         return false;
// //     }
// //     conn_ = *conn_opt;

// //     // 2. Create KCP control block
// //     kcp_ = ikcp_create(conv_id_, &conn_);
// //     if (!kcp_) {
// //         ESP_LOGE(TAG, "Failed to allocate KCP control block.");
// //         close(conn_.sock);
// //         return false;
// //     }

// //     // 3. Wire callbacks & fast-mode settings
// //     ikcp_setoutput(kcp_, kcp_output_callback);
// //     ikcp_nodelay(kcp_, 1, 10, 2, 1);
// //     ikcp_wndsize(kcp_, 128, 128);

// //     ESP_LOGI(TAG, "KCP Client fully initialized and configured!");
// //     return true;
// // }

// // void KcpAudioClient::kcp_task_trampoline(void* arg) {
// //     auto* client = static_cast<KcpAudioClient*>(arg);
// //     client->kcp_worker_loop();
// // }

// // void KcpAudioClient::kcp_worker_loop() {
// //     uint8_t rx_buffer[1024];
// //     struct sockaddr_storage src_addr;
// //     socklen_t src_addr_len;

// //     const TickType_t xFrequency = pdMS_TO_TICKS(10);
// //     TickType_t xLastWakeTime = xTaskGetTickCount();

// //     ESP_LOGI(TAG, "KCP background worker task started.");

// //     while (running_) {
// //         uint32_t current_ms = pdTICKS_TO_MS(xTaskGetTickCount());

// //         // Poll socket for incoming packets
// //         src_addr_len = sizeof(src_addr);
// //         ssize_t bytes_received = udp_receive(conn_.sock, rx_buffer, sizeof(rx_buffer), &src_addr, &src_addr_len);

// //         if (bytes_received > 0) {
// //             ikcp_input(kcp_, reinterpret_cast<const char*>(rx_buffer), bytes_received);
// //         }

// //         // Drive KCP clock
// //         ikcp_update(kcp_, current_ms);

// //         vTaskDelayUntil(&xLastWakeTime, xFrequency);
// //     }

// //     vTaskDelete(nullptr);
// // }

// // bool KcpAudioClient::start_background_worker() {
// //     if (running_) return true;
// //     running_ = true;

// //     BaseType_t res = xTaskCreate(
// //         kcp_task_trampoline,
// //         "kcp_worker",
// //         4096,
// //         this,
// //         5,
// //         &kcp_task_handle_
// //     );

// //     if (res != pdPASS) {
// //         ESP_LOGE(TAG, "Failed to create KCP background task!");
// //         running_ = false;
// //         return false;
// //     }
// //     return true;
// // }

// // void KcpAudioClient::stop_background_worker() {
// //     running_ = false;
// //     vTaskDelay(pdMS_TO_TICKS(20)); // Brief pause for clean task exit
// // }

// // int KcpAudioClient::send_audio_data(const uint8_t* pcm_data, size_t size) {
// //     if (!kcp_) return -1;
// //     return ikcp_send(kcp_, reinterpret_cast<const char*>(pcm_data), size);
// // }

// // int KcpAudioClient::receive_audio_data(uint8_t* buffer, size_t max_size) {
// //     if (!kcp_) return -1;
// //     return ikcp_recv(kcp_, reinterpret_cast<char*>(buffer), max_size);
// // }




// #include "KcpAudioClient.hpp"
// #include <fcntl.h>
// #include <unistd.h>
// #include <errno.h>
// #include <esp_log.h>
// #include <cstring>

// static const char* TAG = "KCP_CLIENT";

// // Helper: Set socket to non-blocking mode
// static bool make_socket_non_blocking(int sock) {
//     int flags = fcntl(sock, F_GETFL, 0);
//     if (flags < 0) {
//         ESP_LOGE(TAG, "Failed to get socket flags: %d", errno);
//         return false;
//     }
//     if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
//         ESP_LOGE(TAG, "Failed to set socket to non-blocking: %d", errno);
//         return false;
//     }
//     return true;
// }

// // Helper: Production UDP socket setup with getaddrinfo (IPv4/IPv6 agnostic)
// static std::optional<UdpConnection> setup_production_udp_socket(const char* host, const char* port) {
//     struct addrinfo hints = {};
//     hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6 dynamically
//     hints.ai_socktype = SOCK_DGRAM;  // UDP datagram socket
//     hints.ai_protocol = IPPROTO_UDP;

//     struct addrinfo* res = nullptr;
//     int err = getaddrinfo(host, port, &hints, &res);
    
//     if (err != 0 || res == nullptr) {
//         ESP_LOGE(TAG, "Failed to resolve hostname %s: error %d", host, err);
//         return std::nullopt;
//     }

//     int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//     if (sock < 0) {
//         ESP_LOGE(TAG, "Failed to create socket for family %d", res->ai_family);
//         freeaddrinfo(res);
//         return std::nullopt;
//     }

//     if (!make_socket_non_blocking(sock)) {
//         close(sock);
//         freeaddrinfo(res);
//         return std::nullopt;
//     }

//     UdpConnection conn;
//     conn.sock = sock;
//     std::memcpy(&conn.addr, res->ai_addr, res->ai_addrlen);
//     conn.addr_len = res->ai_addrlen;

//     freeaddrinfo(res);
//     ESP_LOGI(TAG, "UDP socket successfully created and set to non-blocking.");
//     return conn;
// }

// // Helper: Non-blocking UDP send wrapper
// static bool udp_send(const UdpConnection& conn, const uint8_t* data, size_t len) {
//     if (!conn.is_valid()) {
//         ESP_LOGE(TAG, "Attempted to send on an invalid socket!");
//         return false;
//     }

//     ssize_t sent = sendto(conn.sock, data, len, 0, (struct sockaddr*)&conn.addr, conn.addr_len);
//     if (sent < 0) {
//         if (errno == EWOULDBLOCK || errno == EAGAIN) {
//             ESP_LOGW(TAG, "Send buffer full (EWOULDBLOCK). Packet dropped.");
//             return false;
//         } else {
//             ESP_LOGE(TAG, "sendto failed with error: %d", errno);
//             return false;
//         }
//     }
//     return true;
// }

// // Helper: Non-blocking UDP receive wrapper
// static ssize_t udp_receive(int sock, uint8_t* buffer, size_t max_len, struct sockaddr_storage* src_addr, socklen_t* src_addr_len) {
//     ssize_t received = recvfrom(sock, buffer, max_len, 0, (struct sockaddr*)src_addr, src_addr_len);
//     if (received < 0) {
//         if (errno == EWOULDBLOCK || errno == EAGAIN) {
//             return 0; // No data available right now (normal for non-blocking sockets)
//         } else {
//             ESP_LOGE(TAG, "recvfrom error: %d", errno);
//             return -1;
//         }
//     }
//     return received;
// }

// // KCP Output Bridge Callback
// int KcpAudioClient::kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user) {
//     auto* conn = static_cast<UdpConnection*>(user);
//     if (!conn || !conn->is_valid()) {
//         return -1;
//     }
//     bool success = udp_send(*conn, reinterpret_cast<const uint8_t*>(buf), len);
//     return success ? 0 : -1;
// }

// KcpAudioClient::~KcpAudioClient() {
//     close_connection();
// }

// bool KcpAudioClient::initialize(const char* host, const char* port) {
//     // 1. Establish underlying transport
//     auto conn_opt = setup_production_udp_socket(host, port);
//     if (!conn_opt || !conn_opt->is_valid()) {
//         ESP_LOGE(TAG, "Failed to initialize underlying UDP socket.");
//         return false;
//     }
//     conn_ = *conn_opt;

//     // 2. Create KCP control block
//     kcp_ = ikcp_create(conv_id_, &conn_);
//     if (!kcp_) {
//         ESP_LOGE(TAG, "Failed to allocate KCP control block.");
//         close(conn_.sock);
//         conn_.sock = -1;
//         return false;
//     }

//     // 3. Wire callbacks & fast-mode settings for low-latency audio
//     ikcp_setoutput(kcp_, kcp_output_callback);
//     ikcp_nodelay(kcp_, 1, 10, 2, 1);
//     ikcp_wndsize(kcp_, 128, 128);

//     ESP_LOGI(TAG, "KCP Protocol Client fully initialized and configured!");
//     return true;
// }

// void KcpAudioClient::close_connection() {
//     if (kcp_) {
//         ikcp_release(kcp_);
//         kcp_ = nullptr;
//     }
//     if (conn_.is_valid()) {
//         close(conn_.sock);
//         conn_.sock = -1;
//     }
// }

// void KcpAudioClient::process_incoming() {
//     if (!is_initialized()) return;

//     uint8_t rx_buffer[1024];
//     struct sockaddr_storage src_addr;
//     socklen_t src_addr_len = sizeof(src_addr);

//     ssize_t bytes_received = udp_receive(conn_.sock, rx_buffer, sizeof(rx_buffer), &src_addr, &src_addr_len);

//     if (bytes_received > 0) {
//         int err = ikcp_input(kcp_, reinterpret_cast<const char*>(rx_buffer), bytes_received);
//         if (err < 0) {
//             ESP_LOGW(TAG, "ikcp_input error parsing packet: %d", err);
//         }
//     }
// }

// void KcpAudioClient::update(uint32_t current_ms) {
//     if (kcp_) {
//         ikcp_update(kcp_, current_ms);
//     }
// }

// int KcpAudioClient::send_audio_data(const uint8_t* data, size_t size) {
//     if (!kcp_) return -1;
//     return ikcp_send(kcp_, reinterpret_cast<const char*>(data), size);
// }

// int KcpAudioClient::receive_audio_data(uint8_t* buffer, size_t max_size) {
//     if (!kcp_) return -1;
//     return ikcp_recv(kcp_, reinterpret_cast<char*>(buffer), max_size);
// }












#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <esp_log.h>
#include <cstring>


#include "KcpClient.hpp"
#include "../communicate-data/receive-send-net-data.hpp"
namespace Network {


static const char* TAG = "KCP_CLIENT";

// Helper: Set socket to non-blocking mode
static bool make_socket_non_blocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        ESP_LOGE(TAG, "Failed to get socket flags: %d", errno);
        return false;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        ESP_LOGE(TAG, "Failed to set socket to non-blocking: %d", errno);
        return false;
    }
    return true;
}

// Helper: Production UDP socket setup with getaddrinfo (IPv4/IPv6 agnostic)
static std::optional<UdpConnection> setup_production_udp_socket(const char* host, const char* port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6 dynamically
    hints.ai_socktype = SOCK_DGRAM;  // UDP datagram socket
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo* res = nullptr;
    int err = getaddrinfo(host, port, &hints, &res);
    
    if (err != 0 || res == nullptr) {
        ESP_LOGE(TAG, "Failed to resolve hostname %s: error %d", host, err);
        return std::nullopt;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket for family %d", res->ai_family);
        freeaddrinfo(res);
        return std::nullopt;
    }

    if (!make_socket_non_blocking(sock)) {
        close(sock);
        freeaddrinfo(res);
        return std::nullopt;
    }

    UdpConnection conn;
    conn.sock = sock;
    std::memcpy(&conn.addr, res->ai_addr, res->ai_addrlen);
    conn.addr_len = res->ai_addrlen;

    freeaddrinfo(res);
    ESP_LOGI(TAG, "UDP socket successfully created and set to non-blocking.");
    return conn;
}

// Helper: Non-blocking UDP send wrapper
static bool udp_send(const UdpConnection& conn, const uint8_t* data, size_t len) {
    if (!conn.is_valid()) {
        ESP_LOGE(TAG, "Attempted to send on an invalid socket!");
        return false;
    }

    ssize_t sent = sendto(conn.sock, data, len, 0, (struct sockaddr*)&conn.addr, conn.addr_len);
    if (sent < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            ESP_LOGW(TAG, "Send buffer full (EWOULDBLOCK). Packet dropped.");
            return false;
        } else {
            ESP_LOGE(TAG, "sendto failed with error: %d", errno);
            return false;
        }
    }
    return true;
}


// KCP Output Bridge Callback
static int  kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user) {
    auto* conn = static_cast<UdpConnection*>(user);
    if (!conn || !conn->is_valid()) {
        return -1;
    }
    bool success = udp_send(*conn, reinterpret_cast<const uint8_t*>(buf), len);
    return success ? 0 : -1;
}

KcpClient::~KcpClient() {
    close_connection();
}
std::vector<uint8_t> KcpClient::poll_and_receive() {
    if (!is_initialized()) return {};

    // 1. Fetch raw bytes using our clean helper and feed into KCP
    uint8_t rx_buffer[1024];
    struct sockaddr_storage src_addr;
    socklen_t src_addr_len = sizeof(src_addr);

    ssize_t bytes_received =communicate:: udp_receive(conn_.sock, rx_buffer, sizeof(rx_buffer), &src_addr, &src_addr_len);
    if (bytes_received > 0) {
        int err = ikcp_input(kcp_, reinterpret_cast<const char*>(rx_buffer), bytes_received);
        if (err < 0) {
            ESP_LOGW(TAG, "ikcp_input error parsing packet: %d", err);
        }
    }

    // 2. Check if KCP has a reassembled message ready using ikcp_peeksize
    int peek_size = ikcp_peeksize(kcp_);
    if (peek_size <= 0) {
        return {}; // Return empty vector if nothing is ready yet
    }

    // 3. Allocate vector dynamically to the exact required size safely
    std::vector<uint8_t> buffer(peek_size);
    int bytes_read = ikcp_recv(kcp_, reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (bytes_read < 0) {
        return {};
    }

    buffer.resize(bytes_read);
    return buffer;
}
bool KcpClient::initialize(const char* host, const char* port) {
    // 1. Establish underlying transport
    auto conn_opt = setup_production_udp_socket(host, port);
    if (!conn_opt || !conn_opt->is_valid()) {
        ESP_LOGE(TAG, "Failed to initialize underlying UDP socket.");
        return false;
    }
    conn_ = *conn_opt;

    // 2. Create KCP control block
    kcp_ = ikcp_create(conv_id_, &conn_);
    if (!kcp_) {
        ESP_LOGE(TAG, "Failed to allocate KCP control block.");
        close(conn_.sock);
        conn_.sock = -1;
        return false;
    }

    // 3. Wire callbacks & fast-mode settings for low-latency communication
    ikcp_setoutput(kcp_, kcp_output_callback);
    ikcp_nodelay(kcp_, 1, 10, 2, 1);
    ikcp_wndsize(kcp_, 128, 128);

    ESP_LOGI(TAG, "KCP Client fully initialized and configured!");
    return true;
}

void KcpClient::close_connection() {
    if (kcp_) {
        ikcp_release(kcp_);
        kcp_ = nullptr;
    }
    if (conn_.is_valid()) {
        close(conn_.sock);
        conn_.sock = -1;
    }
}

void KcpClient::update(uint32_t current_ms) {
    if (kcp_) {
        ikcp_update(kcp_, current_ms);
    }
}



int KcpClient::send(const uint8_t* data, size_t size) {
    if (!kcp_) return -1;
    return ikcp_send(kcp_, reinterpret_cast<const char*>(data), size);
}









}