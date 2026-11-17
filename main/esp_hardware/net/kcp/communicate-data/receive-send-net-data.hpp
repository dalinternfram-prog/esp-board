#pragma once

#include <string>
#include <sys/socket.h>
#include <esp_log.h>
#include <errno.h>
#include "../socket/create-udp-socket.hpp"
namespace Network::communicate {
static const char* TAG = "UDP";

/**
 * @brief Sends raw bytes over a non-blocking UDP socket.
 */
bool udp_send(const socket::UdpConnection& conn, const uint8_t* data, size_t len) {
    if (!conn.is_valid() || data == nullptr || len == 0) {
        ESP_LOGE(TAG, "Invalid parameters for send");
        return false;
    }

    ssize_t sent;
    do {
        sent = sendto(
            conn.sock, 
            data, 
            len, 
            0, 
            (struct sockaddr*)&conn.addr, 
            conn.addr_len
        );
    } while (sent < 0 && errno == EINTR);

    if (sent < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            // Buffer full: Handle rate-limiting or drop metrics quietly
            return false;
        }
        ESP_LOGE(TAG, "sendto failed errno: %d", errno);
        return false;
    }
    
//  ESP_LOGW(TAG, "=============================== >Send through UDP size = %zu",sent);
//  std::string client_hex = "";
// for (size_t i = 0; i < sent; ++i) {
//     char buf[3];
//     snprintf(buf, sizeof(buf), "%02x", data[i]);
//     client_hex += buf;
// }
// ESP_LOGI(TAG, "Client Raw Packet (Hex): %s", client_hex.c_str());
    // Ensure entire packet was accepted by system socket buffer
    return (size_t)sent == len;
}

/**
 * @brief Polls the UDP socket for incoming raw bytes.
 * @return >0 for bytes received, 0 for no data available, -1 on socket error.
 */
ssize_t udp_receive(int sock, uint8_t* buffer, size_t max_len, struct sockaddr_storage* src_addr, socklen_t* src_addr_len) {
    if (sock < 0 || buffer == nullptr || max_len == 0 || src_addr == nullptr || src_addr_len == nullptr) {
        return -1;
    }

    ssize_t received;
    do {
        received = recvfrom(
            sock, 
            buffer, 
            max_len, 
            0, 
            (struct sockaddr*)src_addr, 
            src_addr_len
        );
    } while (received < 0 && errno == EINTR);

    if (received < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0; // Expected non-blocking state (no data available)
        }
        ESP_LOGE(TAG, "recvfrom failed errno: %d", errno);
        return -1;
    }

    return received;
}
} ;
