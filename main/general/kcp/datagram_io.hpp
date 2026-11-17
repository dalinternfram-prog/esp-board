#pragma once

// A datagram channel used by the portable KCP session.
// Implementations: ESP UDP socket, PPP, in-memory test loop, ...

#include <cstddef>
#include <cstdint>

namespace general::kcp {

struct DatagramIo {
    virtual ~DatagramIo() = default;

    // Returns 0 on success, < 0 on failure (a non-blocking drop is a failure).
    virtual int send(const uint8_t* data, std::size_t len) = 0;

    // Returns > 0 bytes read, 0 when no datagram is available, < 0 on error.
    virtual int receive(uint8_t* buffer, std::size_t capacity) = 0;
};

} // namespace general::kcp
