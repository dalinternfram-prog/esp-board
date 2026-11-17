#pragma once

// Portable KCP session: control block, fast-mode tuning, send/input/reassembly
// and liveness timestamp. Transport I/O is injected through DatagramIo.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "general/kcp/datagram_io.hpp"
#include "ikcp.h"

namespace general::kcp {

struct KcpConfig {
    uint32_t conversation_id = 0;

    // ikcp_nodelay(nodelay, interval_ms, resend, fast_resend)
    int nodelay     = 1;
    int interval_ms = 10;
    int resend      = 2;
    int fast_resend = 1;

    // ikcp_wndsize(send, receive)
    int send_window    = 128;
    int receive_window = 128;
};

class KcpSession {
public:
    KcpSession(DatagramIo& io, const KcpConfig& config);
    ~KcpSession();

    KcpSession(const KcpSession&) = delete;
    KcpSession& operator=(const KcpSession&) = delete;

    bool open();
    void close();
    bool isOpen() const { return kcp_ != nullptr; }

    int send(const uint8_t* data, std::size_t size);

    // Reads at most one datagram, feeds KCP, and returns one reassembled message
    // (empty when nothing is ready). now_ms is used for the liveness timestamp.
    std::vector<uint8_t> poll(uint32_t now_ms);

    void update(uint32_t now_ms);

    uint32_t lastRxMs() const { return last_rx_ms_; }

private:
    static int outputCallback(const char* buf, int len, ikcpcb* kcp, void* user);

    DatagramIo& io_;
    KcpConfig   config_;
    ikcpcb*     kcp_ = nullptr;
    uint32_t    last_rx_ms_ = 0;
};

} // namespace general::kcp
