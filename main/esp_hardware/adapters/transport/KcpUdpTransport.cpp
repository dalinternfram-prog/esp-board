#include "adapters/transport/KcpUdpTransport.hpp"

#include "esp_hardware/net/kcp/esp_udp_datagram_io.hpp"
#include "esp_timer.h"

namespace adapters {

namespace {
uint32_t nowMs() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}
} // namespace

KcpUdpTransport::KcpUdpTransport(const char* host, const char* port, uint32_t conv_id)
    : host_(host ? host : ""), port_(port ? port : ""), conv_id_(conv_id) {}

KcpUdpTransport::~KcpUdpTransport() { close(); }

bool KcpUdpTransport::open() {
    if (kcp_ && kcp_->isOpen()) {
        return true;
    }

    io_ = std::make_unique<EspUdpDatagramIo>(host_.c_str(), port_.c_str());
    if (!io_->isOpen()) {
        io_.reset();
        return false;
    }

    general::kcp::KcpConfig config;
    config.conversation_id = conv_id_;
    kcp_ = std::make_unique<general::kcp::KcpSession>(*io_, config);
    if (!kcp_->open()) {
        kcp_.reset();
        io_.reset();
        return false;
    }
    return true;
}

void KcpUdpTransport::close() {
    if (kcp_) {
        kcp_->close();
        kcp_.reset();
    }
    io_.reset();
}

bool KcpUdpTransport::isOpen() const {
    return kcp_ && kcp_->isOpen();
}

void KcpUdpTransport::reset() {
    // Recreate the socket + KCP state so the stream restarts cleanly.
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
    if (!io_ || data.empty()) {
        return -1;
    }
    return io_->send(data.data(), data.size());
}

std::optional<core::Bytes> KcpUdpTransport::receive() {
    if (!kcp_) {
        return std::nullopt;
    }
    auto message = kcp_->poll(nowMs());
    if (message.empty()) {
        return std::nullopt;
    }
    return message;
}

uint32_t KcpUdpTransport::lastRxMs() const {
    return kcp_ ? kcp_->lastRxMs() : 0;
}

} // namespace adapters
