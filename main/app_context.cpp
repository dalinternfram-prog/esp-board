#include "app_context.hpp"

#include "sdkconfig.h"

#ifndef CONFIG_APP_SERVER_HOST
#define CONFIG_APP_SERVER_HOST "172.30.165.29"
#endif
#ifndef CONFIG_APP_SERVER_PORT
#define CONFIG_APP_SERVER_PORT "8080"
#endif

namespace app {

AppContext::AppContext()
    : network(*this, link, clock, random, crypto, tasks, logger),
      audio(audio_in, audio_encoder, clock, tasks, logger, network),
      loop(network, audio, device, files, firmware, link, clock, tasks, logger,
           storage, crypto, CONFIG_APP_SERVER_HOST, CONFIG_APP_SERVER_PORT) {}

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
