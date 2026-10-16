#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <esp_log.h>
#include <cstring>
#include <optional>
namespace Network::socket {

// Structure to hold the socket handle and target destination data
struct UdpConnection {
    int sock = -1;
    struct sockaddr_storage addr = {};
    socklen_t addr_len = 0;

    bool is_valid() const { return sock >= 0; }
};

/**
 * @brief Sets a socket file descriptor to non-blocking mode.
 */
bool make_socket_non_blocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        ESP_LOGE("UDP", "Failed to get socket flags: %d", errno);
        return false;
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        ESP_LOGE("UDP", "Failed to set socket to non-blocking: %d", errno);
        return false;
    }

    ESP_LOGI("UDP", "Socket successfully set to non-blocking mode.");
    return true;
}

/**
 * @brief Resolves a domain/IP (IPv4/IPv6), creates a UDP socket, and configures it.
 */
std::optional<UdpConnection> setup_production_udp_socket(const char* host, const char* port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6 dynamically
    hints.ai_socktype = SOCK_DGRAM;  // UDP datagram socket
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo* res = nullptr;
    int err = getaddrinfo(host, port, &hints, &res);
    
    if (err != 0 || res == nullptr) {
        ESP_LOGE("UDP", "Failed to resolve hostname %s: error %d", host, err);
        return std::nullopt;
    }

    // 1. Create the socket using the family discovered by getaddrinfo
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        ESP_LOGE("UDP", "Failed to create socket for family %d", res->ai_family);
        freeaddrinfo(res);
        return std::nullopt;
    }

    // 2. Make the socket non-blocking using our helper function
    if (!make_socket_non_blocking(sock)) {
        close(sock);
        freeaddrinfo(res);
        return std::nullopt;
    }

    // 3. Populate our connection struct with the resolved destination info
    UdpConnection conn;
    conn.sock = sock;
    std::memcpy(&conn.addr, res->ai_addr, res->ai_addrlen);
    conn.addr_len = res->ai_addrlen;

    // Clean up memory allocated by getaddrinfo
    freeaddrinfo(res);

    ESP_LOGI("UDP", "Production UDP socket successfully created and configured!");
    return conn;
}
}