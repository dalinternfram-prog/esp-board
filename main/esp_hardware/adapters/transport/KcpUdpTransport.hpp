#pragma once

// Adapter: implements core::ITransport on top of the existing UDP + KCP client.
// This is the only place that knows about KCP/UDP. A 4G/PPP transport would be
// a sibling adapter implementing the same port.

#include "interface.hpp"

#include <memory>
#include <string>

namespace Network {
class KcpClient;
}

namespace adapters {

class KcpUdpTransport : public core::ITransport {
public:
    KcpUdpTransport(const char* host, const char* port, uint32_t conv_id);
    ~KcpUdpTransport() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    void reset() override;
    void tick(uint32_t now_ms) override;
    int sendReliable(const core::Bytes& data) override;
    int sendRaw(const core::Bytes& data) override;
    std::optional<core::Bytes> receive() override;
    uint32_t lastRxMs() const override;
    uint32_t conversationId() const override { return conv_id_; }

private:
    std::string host_;
    std::string port_;
    uint32_t    conv_id_;
    std::unique_ptr<Network::KcpClient> kcp_;
};

} // namespace adapters
