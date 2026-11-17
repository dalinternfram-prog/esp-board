#include "general/kcp/kcp_session.hpp"

namespace general::kcp {

KcpSession::KcpSession(DatagramIo& io, const KcpConfig& config)
    : io_(io), config_(config) {}

KcpSession::~KcpSession() {
    close();
}

bool KcpSession::open() {
    if (kcp_ != nullptr) {
        return true;
    }

    kcp_ = ikcp_create(config_.conversation_id, this);
    if (kcp_ == nullptr) {
        return false;
    }

    ikcp_setoutput(kcp_, &KcpSession::outputCallback);
    ikcp_nodelay(kcp_, config_.nodelay, config_.interval_ms,
                 config_.resend, config_.fast_resend);
    ikcp_wndsize(kcp_, config_.send_window, config_.receive_window);
    return true;
}

void KcpSession::close() {
    if (kcp_ != nullptr) {
        ikcp_release(kcp_);
        kcp_ = nullptr;
    }
}

int KcpSession::outputCallback(const char* buf, int len, ikcpcb* /*kcp*/, void* user) {
    auto* self = static_cast<KcpSession*>(user);
    if (self == nullptr || buf == nullptr || len <= 0) {
        return -1;
    }
    return self->io_.send(reinterpret_cast<const uint8_t*>(buf),
                          static_cast<std::size_t>(len));
}

int KcpSession::send(const uint8_t* data, std::size_t size) {
    if (kcp_ == nullptr || data == nullptr || size == 0) {
        return -1;
    }
    return ikcp_send(kcp_, reinterpret_cast<const char*>(data),
                     static_cast<int>(size));
}

std::vector<uint8_t> KcpSession::poll(uint32_t now_ms) {
    if (kcp_ == nullptr) {
        return {};
    }

    uint8_t rx_buffer[1500];
    const int received = io_.receive(rx_buffer, sizeof(rx_buffer));
    if (received > 0) {
        last_rx_ms_ = now_ms;
        (void)ikcp_input(kcp_, reinterpret_cast<const char*>(rx_buffer), received);
    }

    const int peek_size = ikcp_peeksize(kcp_);
    if (peek_size <= 0) {
        return {};
    }

    std::vector<uint8_t> buffer(static_cast<std::size_t>(peek_size));
    const int bytes_read = ikcp_recv(kcp_, reinterpret_cast<char*>(buffer.data()),
                                     static_cast<int>(buffer.size()));
    if (bytes_read < 0) {
        return {};
    }

    buffer.resize(static_cast<std::size_t>(bytes_read));
    return buffer;
}

void KcpSession::update(uint32_t now_ms) {
    if (kcp_ != nullptr) {
        ikcp_update(kcp_, now_ms);
    }
}

} // namespace general::kcp
