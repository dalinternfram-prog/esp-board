#pragma once

// Composition root: owns every concrete adapter, the network/audio services
// and the application loop. A plain class created in main.cpp; there is no
// global singleton accessor and no service locator.

#include <memory>

#include "core/app/audio_service.hpp"
#include "core/app/media_controller.hpp"
#include "core/app/network_service.hpp"
#include "core/app/prog.hpp"
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

    core::app::NetworkService network;
    core::app::AudioService   audio;
    core::app::AppLoop        loop;

    adapters::EspMediaPlayer media_player;
    adapters::EspIndicator   indicator;
    adapters::EspButtonInput buttons;
    adapters::EspSpeechInput speech;
    adapters::EspSdPlaylist  playlist;

    std::unique_ptr<core::app::MediaController> media;
};

} // namespace app
