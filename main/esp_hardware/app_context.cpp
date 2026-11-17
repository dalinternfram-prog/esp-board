#include "app_context.hpp"

#include "core/app/session_factory.hpp"

namespace app {

AppContext& context() {
    static AppContext ctx;
    return ctx;
}

void AppContext::buildSession(const char* host, const char* port) {
    const uint32_t conv_id = core::app::nextConversationId(random);
    transport = std::make_unique<adapters::KcpUdpTransport>(host, port, conv_id);
    session = core::app::makeSession(*transport, link, clock, random, crypto, logger);
    session->start();
}

void AppContext::startMedia() {
    if (media) {
        return;
    }

    media = std::make_unique<core::app::MediaController>(
        media_player, indicator, buttons, speech, playlist, logger);

    core::app::MediaConfig config;
    if (!media->start(config)) {
        logger.logf(core::LogLevel::Error, "Media controller failed to start");
    }
}

} // namespace app
