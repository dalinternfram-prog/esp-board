#include "core/app/session_factory.hpp"

namespace core::app {

uint32_t nextConversationId(core::IRandom& random) {
    return random.u32();
}

std::unique_ptr<core::SessionManager> makeSession(core::ITransport& transport,
                                                  core::ILink& link,
                                                  core::IClock& clock,
                                                  core::IRandom& random,
                                                  core::ICrypto& crypto,
                                                  core::ILogger& logger) {
    return std::make_unique<core::SessionManager>(
        transport, link, clock, random, crypto, logger);
}

} // namespace core::app
