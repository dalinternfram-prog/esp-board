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
constexpr uint32_t kKeepaliveMs = 3000;  // 3 s: >=2 round-trips of margin vs the 8 s timeout
constexpr const char* kHost = "172.30.165.156";
constexpr const char* kPort = "8080";

void handle_incoming(const core::Bytes& packet, core::IAppServices& svc,
                     bool& firmware_marked) {
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

            // The device reached the server and works: cancel OTA rollback
            // exactly once.
            if (!firmware_marked && svc.firmware.markAppValid()) {
                firmware_marked = true;
                svc.log.logf(core::LogLevel::Info, "Firmware marked valid");
            }
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

    // Registration is sent on (re)connect transitions and then as a 3 s
    // keepalive; it is never spammed from the fast loop.
    bool registered = false;
    bool firmware_marked = false;
    uint32_t last_epoch = 0;
    uint32_t last_registration_ms = 0;

    while (true) {
        // Reconnect detection via the session epoch: every reset bumps the
        // epoch, so a fast Established->reset->Established cycle is never missed.
        const uint32_t epoch = Network::session_epoch();
        if (epoch != last_epoch) {
            last_epoch = epoch;
            registered = false;

            // A new server session must not inherit a running capture: the
            // server has to send a fresh START command after every reconnect.
            if (AudioService::is_running()) {
                AudioService::stop();
                svc->log.logf(core::LogLevel::Info, "Audio stopped after session reset");
            }
        }

        const bool established = Network::is_established();
        if (established &&
            (!registered ||
             (svc->clock.nowMs() - last_registration_ms) >= kKeepaliveMs)) {
            Network::send(registration);
            registered = true;
            last_registration_ms = svc->clock.nowMs();
        }

        // Drain everything the session produced since the last tick.
        core::Bytes incoming;
        while (!(incoming = Network::poll_and_receive()).empty()) {
            handle_incoming(incoming, *svc, firmware_marked);
        }

        svc->tasks.delayMs(kLoopDelayMs);   // yield to keep the task watchdog happy
    }

    svc->tasks.exitCurrentTask();           // unreachable; documents task lifecycle
}
