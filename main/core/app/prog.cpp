// Application: device registration + server message dispatch.
// Receives core::IAppServices from the platform layer; no hardware/root includes.

#include <cstdint>
#include <string>
#include <vector>

#include "core/app/audio_service.hpp"
#include "core/app/net_manager.hpp"
#include "core/app/prog.hpp"

#include "IoT_identity_token.hpp"   // protocol::IoT::client device-query encode/decode
#include "interface.hpp"
#include "turn_on_audio.hpp"        // protocol::IoT record-control encode/decode

namespace {

constexpr uint32_t kLoopDelayMs = 10;
constexpr const char* kHost = "172.30.165.156";
constexpr const char* kPort = "8080";

void handle_incoming(const core::Bytes& packet, core::IAppServices& svc) {
    // 1. Record start/stop commands from the server.
    if (protocol::IoT::client::isRecordControl(packet)) {
        protocol::IoT::RecordControlInfo info;
        if (!protocol::IoT::client::decodeRecordControl(packet, info)) {
            svc.log.logf(core::LogLevel::Warn, "Failed to decode valid RecordControl payload.");
            return;
        }

        if (info.command == protocol::IoT::RecordCommand::START) {
            if (!AudioService::is_running()) {
                AudioService::start(svc);
                svc.log.logf(core::LogLevel::Info,
                             "Audio service started via server command (timestamp %u)",
                             static_cast<unsigned>(info.timestampMs));
            }
        } else if (info.command == protocol::IoT::RecordCommand::STOP) {
            if (AudioService::is_running()) {
                AudioService::stop();
                svc.log.logf(core::LogLevel::Info,
                             "Audio service stopped via server command.");
            }
        }
        return;
    }

    // 2. Device-query acknowledgement.
    if (protocol::IoT::client::isDeviceQueryResponse(packet)) {
        protocol::IoT::BuisDeviceQueryResponse response;
        if (!protocol::IoT::client::decodeDeviceQueryResponse(packet, response)) {
            svc.log.logf(core::LogLevel::Warn, "Failed to decode BuisDeviceQueryResponse payload.");
            return;
        }
        if (response.status == protocol::IoT::Status_DeviceQuery::SUCCESS) {
            svc.log.logf(core::LogLevel::Info, "Device query acknowledged: %s",
                         response.message.c_str());
        } else {
            svc.log.logf(core::LogLevel::Warn, "Device query rejected (status %u): %s",
                         static_cast<unsigned>(response.status), response.message.c_str());
        }
        return;
    }

    svc.log.logf(core::LogLevel::Info, "Received general server message or unhandled packet type.");
}

} // namespace

void app_run_loop(void* arg) {
    auto* svc = static_cast<core::IAppServices*>(arg);
    if (svc == nullptr) {
        return;
    }

    if (!svc->files.init()) {
        svc->log.logf(core::LogLevel::Warn, "Filesystem mount failed");
    }
    svc->firmware.init();
    svc->link.start();                 // WiFi provisioning / association
    Network::init(kHost, kPort, *svc);

    const std::string device_id = svc->device.deviceId();
    const std::vector<uint8_t> registration =
        protocol::IoT::client::encodeDeviceQuery(device_id);

    Network::send(registration);
    svc->log.logf(core::LogLevel::Info,
                  "Sent device registration query for ID: %s", device_id.c_str());

    while (true) {
        Network::send(registration);

        const core::Bytes incoming = Network::poll_and_receive();
        if (!incoming.empty()) {
            handle_incoming(incoming, *svc);
        }

        svc->tasks.delayMs(kLoopDelayMs);   // yield to keep the task watchdog happy
    }

    svc->tasks.exitCurrentTask();           // unreachable; documents task lifecycle
}
