#pragma once

// Adapter: implements core::ITransport on top of the portable KCP session and
// the ESP UDP datagram channel. KCP logic lives in general/kcp/KcpSession.

#include "interface.hpp"

#include <memory>
#include <string>

#include "general/kcp/kcp_session.hpp"

namespace adapters {

class EspUdpDatagramIo;

class KcpUdpTransport : public core::ITransport {
public:
    KcpUdpTransport(const char* host, const char* port, uint32_t conv_id);
    ~KcpUdpTransport() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    void reset(uint32_t new_conv_id) override;
    void tick(uint32_t now_ms) override;
    int sendReliable(const core::Bytes& data) override;
    int sendRaw(const core::Bytes& data) override;
    std::optional<core::Bytes> receive(uint32_t now_ms) override;
    uint32_t lastRxMs() const override;
    bool isDead() const override { return kcp_ && kcp_->isDead(); }
    uint32_t conversationId() const override { return conv_id_; }

private:
    std::string host_;
    std::string port_;
    uint32_t    conv_id_;

    std::unique_ptr<EspUdpDatagramIo>         io_;
    std::unique_ptr<general::kcp::KcpSession> kcp_;
};

} // namespace adapters

