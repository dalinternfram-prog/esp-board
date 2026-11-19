#pragma once

// Portable POSIX UDP socket helpers (getaddrinfo / socket / sendto / recvfrom).
// Works on ESP-IDF lwIP and on any POSIX host; no ESP-IDF includes. The
// hardware layer consumes this through its DatagramIo adapter.

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>

#include "general/log.hpp"

namespace general::net {

inline constexpr const char* TAG = "UDP";

// Socket handle + resolved destination.
struct UdpConnection {
    int sock = -1;
    struct sockaddr_storage addr = {};
    socklen_t addr_len = 0;

    bool is_valid() const { return sock >= 0; }
};

inline bool makeSocketNonBlocking(int sock) {
    const int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        LOG_E(TAG, "Failed to get socket flags: %d", errno);
        return false;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_E(TAG, "Failed to set socket to non-blocking: %d", errno);
        return false;
    }
    return true;
}

// Resolves the host (IPv4/IPv6), creates a non-blocking UDP socket.
inline std::optional<UdpConnection> openUdpConnection(const char* host, const char* port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo* res = nullptr;
    const int err = getaddrinfo(host, port, &hints, &res);
    if (err != 0 || res == nullptr) {
        LOG_E(TAG, "Failed to resolve hostname %s: error %d", host, err);
        return std::nullopt;
    }

    const int sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        LOG_E(TAG, "Failed to create socket for family %d", res->ai_family);
        freeaddrinfo(res);
        return std::nullopt;
    }

    if (!makeSocketNonBlocking(sock)) {
        ::close(sock);
        freeaddrinfo(res);
        return std::nullopt;
    }

    UdpConnection conn;
    conn.sock = sock;
    std::memcpy(&conn.addr, res->ai_addr, res->ai_addrlen);
    conn.addr_len = res->ai_addrlen;

    // Connected UDP: only datagrams from the resolved peer are delivered, so
    // stray LAN traffic never reaches ikcp_input().
    if (::connect(sock, reinterpret_cast<const struct sockaddr*>(&conn.addr),
                  conn.addr_len) != 0) {
        LOG_E(TAG, "Failed to connect UDP socket: %d", errno);
        ::close(sock);
        freeaddrinfo(res);
        return std::nullopt;
    }

    freeaddrinfo(res);
    return conn;
}

// Returns 0 on success, -1 on failure (a non-blocking drop is a failure).
inline int udpSend(const UdpConnection& conn, const uint8_t* data, size_t len) {
    if (!conn.is_valid() || data == nullptr || len == 0) {
        LOG_E(TAG, "Invalid parameters for send");
        return -1;
    }

    ssize_t sent;
    do {
        sent = sendto(conn.sock, data, len, 0,
                      reinterpret_cast<const struct sockaddr*>(&conn.addr),
                      conn.addr_len);
    } while (sent < 0 && errno == EINTR);

    if (sent < 0) {
        return -1;   // includes EWOULDBLOCK/EAGAIN (buffer full -> drop)
    }
    return (static_cast<size_t>(sent) == len) ? 0 : -1;
}

// Returns > 0 bytes, 0 when no datagram is available, -1 on error.
inline int udpReceive(int sock, uint8_t* buffer, size_t capacity,
                      struct sockaddr_storage* src_addr, socklen_t* src_addr_len) {
    if (sock < 0 || buffer == nullptr || capacity == 0 ||
        src_addr == nullptr || src_addr_len == nullptr) {
        return -1;
    }

    ssize_t received;
    do {
        received = recvfrom(sock, buffer, capacity, 0,
                            reinterpret_cast<struct sockaddr*>(src_addr),
                            src_addr_len);
    } while (received < 0 && errno == EINTR);

    if (received < 0) {
        return (errno == EWOULDBLOCK || errno == EAGAIN) ? 0 : -1;
    }
    return static_cast<int>(received);
}

} // namespace general::net
