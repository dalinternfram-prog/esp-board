#pragma once

#include <cstdint>
#include <vector>

namespace core { struct IAppServices; }

namespace Network {

    // Builds the transport/session from the injected services and starts the
    // network worker task.
    void init(const char* host, const char* port, core::IAppServices& services);
    void close_connection();

    // Data Flow APIs (Thread-safe, multi-producer compatible)
    int send(const std::vector<uint8_t>& data);
    std::vector<uint8_t> poll_and_receive();

    // Session status for the application loop (locked internally).
    bool is_established();
    uint32_t session_epoch();

} // namespace Network
