#pragma once

// Session construction: conv id + SessionManager wiring, independent of which
// transport/link implementation the hardware layer supplied.

#include <cstdint>
#include <memory>

#include "core/session/SessionManager.hpp"
#include "interface.hpp"

namespace core::app {

uint32_t nextConversationId(core::IRandom& random);

std::unique_ptr<core::SessionManager> makeSession(core::ITransport& transport,
                                                  core::ILink& link,
                                                  core::IClock& clock,
                                                  core::IRandom& random,
                                                  core::ICrypto& crypto,
                                                  core::ILogger& logger);

} // namespace core::app
