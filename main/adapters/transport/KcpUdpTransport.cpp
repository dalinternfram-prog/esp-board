#include "adapters/transport/KcpUdpTransport.hpp"

#include "KcpClient.hpp"

namespace adapters {

KcpUdpTransport::KcpUdpTransport(const char* host, const char* port, uint32_t conv_id)
    : host_(host ? host : ""), port_(port ? port : ""), conv_id_(conv_id) {}

KcpUdpTransport::~KcpUdpTransport() { close(); }

bool KcpUdpTransport::open() {
    if (kcp_ && kcp_->is_initialized()) {
        return true;
    }
    kcp_ = std::make_unique<Network::KcpClient>(conv_id_);
    if (!kcp_->initialize(host_.c_str(), port_.c_str())) {
        kcp_.reset();
        return false;
    }
    return true;
}

void KcpUdpTransport::close() {
    if (kcp_) {
        kcp_->close_connection();
        kcp_.reset();
    }
}

bool KcpUdpTransport::isOpen() const {
    return kcp_ && kcp_->is_initialized();
}

void KcpUdpTransport::reset() {
    // Recreate the whole client (socket + KCP) so the stream restarts cleanly.
    close();
    open();
}

void KcpUdpTransport::tick(uint32_t now_ms) {
    if (kcp_) {
        kcp_->update(now_ms);
    }
}

int KcpUdpTransport::sendReliable(const core::Bytes& data) {
    if (!kcp_ || data.empty()) {
        return -1;
    }
    return kcp_->send(data.data(), data.size());
}

int KcpUdpTransport::sendRaw(const core::Bytes& data) {
    if (!kcp_ || data.empty()) {
        return -1;
    }
    return kcp_->raw_udp_send(data.data(), data.size());
}

std::optional<core::Bytes> KcpUdpTransport::receive() {
    if (!kcp_) {
        return std::nullopt;
    }
    auto message = kcp_->poll_and_receive();
    if (message.empty()) {
        return std::nullopt;
    }
    return message;
}

uint32_t KcpUdpTransport::lastRxMs() const {
    return kcp_ ? kcp_->lastRxMs() : 0;
}

} // namespace adapters
