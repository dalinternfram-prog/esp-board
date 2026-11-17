// Application: device registration + server message dispatch.
// Pure C++: every platform capability is reached through the core ports that
// app::context() exposes (no ESP-IDF / FreeRTOS includes in this file).

#include <cstdint>
#include <string>
#include <vector>

#include "app/audio_service.hpp"
#include "app_context.hpp"

#include "IoT_identity_token.hpp"   // protocol::IoT::client device-query encode/decode
#include "network/net_manager.hpp"
#include "turn_on_audio.hpp"        // protocol::IoT record-control encode/decode

namespace {

constexpr uint32_t kLoopDelayMs = 10;
constexpr const char* kHost = "172.30.165.156";
constexpr const char* kPort = "8080";

void handle_incoming(const core::Bytes& packet, core::ILogger& log) {
    // 1. Record start/stop commands from the server.
    if (protocol::IoT::client::isRecordControl(packet)) {
        protocol::IoT::RecordControlInfo info;
        if (!protocol::IoT::client::decodeRecordControl(packet, info)) {
            log.logf(core::LogLevel::Warn, "Failed to decode valid RecordControl payload.");
            return;
        }

        if (info.command == protocol::IoT::RecordCommand::START) {
            if (!AudioService::is_running()) {
                AudioService::start();
                log.logf(core::LogLevel::Info,
                         "Audio service started via server command (timestamp %u)",
                         static_cast<unsigned>(info.timestampMs));
            }
        } else if (info.command == protocol::IoT::RecordCommand::STOP) {
            if (AudioService::is_running()) {
                AudioService::stop();
                log.logf(core::LogLevel::Info,
                         "Audio service stopped via server command.");
            }
        }
        return;
    }

    // 2. Device-query acknowledgement.
    if (protocol::IoT::client::isDeviceQueryResponse(packet)) {
        protocol::IoT::BuisDeviceQueryResponse response;
        if (!protocol::IoT::client::decodeDeviceQueryResponse(packet, response)) {
            log.logf(core::LogLevel::Warn, "Failed to decode BuisDeviceQueryResponse payload.");
            return;
        }
        if (response.status == protocol::IoT::Status_DeviceQuery::SUCCESS) {
            log.logf(core::LogLevel::Info, "Device query acknowledged: %s",
                     response.message.c_str());
        } else {
            log.logf(core::LogLevel::Warn, "Device query rejected (status %u): %s",
                     static_cast<unsigned>(response.status), response.message.c_str());
        }
        return;
    }

    log.logf(core::LogLevel::Info, "Received general server message or unhandled packet type.");
}

} // namespace

// Task entry, started by the platform layer (main.cpp) through ITaskRunner.
void app_run_loop(void* /*arg*/) {
    auto& ctx = app::context();

    if (!ctx.files.init()) {
        ctx.logger.logf(core::LogLevel::Warn, "Filesystem mount failed");
    }
    ctx.firmware.init();
    ctx.link.start();                 // WiFi provisioning / association
    Network::init(kHost, kPort);

    const std::string device_id = ctx.device.deviceId();
    const std::vector<uint8_t> registration =
        protocol::IoT::client::encodeDeviceQuery(device_id);

    Network::send(registration);
    ctx.logger.logf(core::LogLevel::Info,
                    "Sent device registration query for ID: %s", device_id.c_str());

    while (true) {
        Network::send(registration);

        const core::Bytes incoming = Network::poll_and_receive();
        if (!incoming.empty()) {
            handle_incoming(incoming, ctx.logger);
        }

        ctx.tasks.delayMs(kLoopDelayMs);   // yield to keep the task watchdog happy
    }

    ctx.tasks.exitCurrentTask();           // unreachable; documents task lifecycle
}
