#include "esp_hardware/net/kcp/esp_udp_datagram_io.hpp"

#include "communicate-data/receive-send-net-data.hpp"

namespace adapters {

EspUdpDatagramIo::EspUdpDatagramIo(const char* host, const char* port) {
    auto connection = Network::socket::setup_production_udp_socket(host, port);
    if (connection.has_value()) {
        conn_ = *connection;
    }
}

int EspUdpDatagramIo::send(const uint8_t* data, std::size_t len) {
    return Network::communicate::udp_send(conn_, data, len) ? 0 : -1;
}

int EspUdpDatagramIo::receive(uint8_t* buffer, std::size_t capacity) {
    struct sockaddr_storage src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    const ssize_t received = Network::communicate::udp_receive(
        conn_.sock, buffer, capacity, &src_addr, &src_addr_len);
    return static_cast<int>(received);
}

} // namespace adapters
