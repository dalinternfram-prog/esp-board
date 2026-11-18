#pragma once

// Composition root: owns every concrete adapter and the media controller.
// This is a plain class created in main.cpp and shared as core::IAppServices;
// there is no global singleton accessor.

#include <memory>

#include "core/app/media_controller.hpp"
#include "interface.hpp"

#include "esp_hardware/adapters/audio/EspI2sAudioSource.hpp"
#include "esp_hardware/adapters/audio/EspMediaPlayer.hpp"
#include "esp_hardware/adapters/audio/OpusEncoder.hpp"
#include "esp_hardware/adapters/crypto/EspCrypto.hpp"
#include "esp_hardware/adapters/device/EspDeviceInfo.hpp"
#include "esp_hardware/adapters/input/EspButtonInput.hpp"
#include "esp_hardware/adapters/input/EspSpeechInput.hpp"
#include "esp_hardware/adapters/link/EspWifiLink.hpp"
#include "esp_hardware/adapters/ota/EspFirmwareUpdater.hpp"
#include "esp_hardware/adapters/platform/EspPlatform.hpp"
#include "esp_hardware/adapters/platform/EspTaskRunner.hpp"
#include "esp_hardware/adapters/storage/EspFileStore.hpp"
#include "esp_hardware/adapters/storage/EspNvsStorage.hpp"
#include "esp_hardware/adapters/storage/EspSdPlaylist.hpp"
#include "esp_hardware/adapters/transport/KcpUdpTransport.hpp"
#include "esp_hardware/adapters/ui/EspIndicator.hpp"

namespace app {

class AppContext : public core::ITransportFactory {
public:
    AppContext();

    // Port bundle handed to core/app. No singleton: the owner passes this.
    core::IAppServices& services() { return services_; }

    void startMedia();

    // core::ITransportFactory
    std::unique_ptr<core::ITransport> create(const char* host, const char* port,
                                             uint32_t conv_id) override;

    core::AudioConfig audio_config{};

    adapters::EspClock           clock;
    adapters::EspRandom          random;
    adapters::EspLogger          logger;
    adapters::EspCrypto          crypto;
    adapters::EspDeviceInfo      device;
    adapters::EspNvsStorage      storage{"storage"};
    adapters::EspFileStore       files;
    adapters::EspWifiLink        link;
    adapters::EspTaskRunner      tasks;
    adapters::EspFirmwareUpdater firmware;

    adapters::EspI2sAudioSource audio_in{audio_config};
    adapters::OpusEncoder       audio_encoder{audio_config};

    adapters::EspMediaPlayer media_player;
    adapters::EspIndicator   indicator;
    adapters::EspButtonInput buttons;
    adapters::EspSpeechInput speech;
    adapters::EspSdPlaylist  playlist;

    std::unique_ptr<core::app::MediaController> media;

private:
    core::IAppServices services_;
};

} // namespace app
