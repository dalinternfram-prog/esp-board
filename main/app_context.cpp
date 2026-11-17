#include "app_context.hpp"

namespace app {

AppContext& context() {
    static AppContext ctx;
    return ctx;
}

void AppContext::buildSession(const char* host, const char* port) {
    const uint32_t conv_id = random.u32();
    transport = std::make_unique<adapters::KcpUdpTransport>(host, port, conv_id);
    session = std::make_unique<core::SessionManager>(
        *transport, link, clock, random, crypto, logger);
    session->start();
}

} // namespace app
