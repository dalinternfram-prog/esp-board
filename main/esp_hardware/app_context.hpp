#pragma once

// Composition root: the single place where concrete adapters are constructed
// and owned. Everything else depends only on the core ports.
//
// Swapping WiFi -> 4G or ESP -> another MCU means changing this file (and the
// adapter headers it includes) only.

#include <memory>

#include "adapters/audio/EspI2sAudioSource.hpp"
#include "adapters/audio/OpusEncoder.hpp"
#include "adapters/crypto/EspCrypto.hpp"
#include "adapters/device/EspDeviceInfo.hpp"
#include "adapters/link/EspWifiLink.hpp"
#include "adapters/ota/EspFirmwareUpdater.hpp"
#include "adapters/platform/EspPlatform.hpp"
#include "adapters/platform/EspTaskRunner.hpp"
#include "adapters/storage/EspFileStore.hpp"
#include "adapters/storage/EspNvsStorage.hpp"
#include "adapters/transport/KcpUdpTransport.hpp"
#include "core/session/SessionManager.hpp"

namespace app {

struct AppContext {
    // Platform / hardware adapters
    adapters::EspClock      clock;
    adapters::EspRandom     random;
    adapters::EspLogger     logger;
    adapters::EspCrypto     crypto;
    adapters::EspDeviceInfo device;
    adapters::EspNvsStorage storage{"storage"};
    adapters::EspFileStore  files;
    adapters::EspWifiLink   link;
    adapters::EspTaskRunner tasks;
    adapters::EspFirmwareUpdater firmware;

    // Audio chain (hardware is only touched on open() / init()).
    adapters::EspI2sAudioSource audio_in;
    adapters::OpusEncoder       audio_encoder;

    // Network session (built by Network::init).
    std::unique_ptr<adapters::KcpUdpTransport> transport;
    std::unique_ptr<core::SessionManager>      session;

    void buildSession(const char* host, const char* port);
};

// Process-wide composition root (function-local static, built on first use).
AppContext& context();

} // namespace app
