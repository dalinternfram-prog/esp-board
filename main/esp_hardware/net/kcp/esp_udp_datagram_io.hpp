#pragma once

// Adapter: DatagramIo over the ESP-IDF/POSIX UDP socket helpers.

#include <cstddef>

#include "general/kcp/datagram_io.hpp"
#include "socket/create-udp-socket.hpp"

namespace adapters {

class EspUdpDatagramIo : public general::kcp::DatagramIo {
public:
    EspUdpDatagramIo(const char* host, const char* port);

    int send(const uint8_t* data, std::size_t len) override;
    int receive(uint8_t* buffer, std::size_t capacity) override;

    bool isOpen() const { return conn_.is_valid(); }

private:
    Network::socket::UdpConnection conn_;
};

} // namespace adapters
