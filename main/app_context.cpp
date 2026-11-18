#include "app_context.hpp"

namespace app {

AppContext::AppContext()
    : services_{
          logger, clock, random, crypto, device, storage, files, link, tasks,
          firmware, audio_in, audio_encoder, media_player, indicator, buttons,
          speech, playlist, *this} {}

std::unique_ptr<core::ITransport> AppContext::create(const char* host, const char* port,
                                                     uint32_t conv_id) {
    return std::make_unique<adapters::KcpUdpTransport>(host, port, conv_id);
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
