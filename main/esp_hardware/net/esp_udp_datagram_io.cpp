#include "esp_hardware/net/esp_udp_datagram_io.hpp"

namespace adapters {

EspUdpDatagramIo::EspUdpDatagramIo(const char* host, const char* port) {
    auto connection = general::net::openUdpConnection(host, port);
    if (connection.has_value()) {
        conn_ = *connection;
    }
}

EspUdpDatagramIo::~EspUdpDatagramIo() {
    if (conn_.is_valid()) {
        ::close(conn_.sock);
        conn_.sock = -1;
    }
}

int EspUdpDatagramIo::send(const uint8_t* data, std::size_t len) {
    return general::net::udpSend(conn_, data, len);
}

int EspUdpDatagramIo::receive(uint8_t* buffer, std::size_t capacity) {
    struct sockaddr_storage src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    return general::net::udpReceive(conn_.sock, buffer, capacity,
                                    &src_addr, &src_addr_len);
}

} // namespace adapters
