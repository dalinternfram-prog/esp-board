#pragma once

#include <cstdint>
#include <vector>

namespace Network {

    // Lifecycle Management
    void init(const char* host, const char* port);
    void close_connection();

    // Data Flow APIs (Thread-safe, multi-producer compatible)
    int send(const std::vector<uint8_t>& data);
    std::vector<uint8_t> poll_and_receive();

} // namespace Network