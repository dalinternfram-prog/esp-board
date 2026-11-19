// Application loop: device registration + server message dispatch.
// Everything arrives as injected ports; no hardware/root includes.

#include "core/app/prog.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "IoT_identity_token.hpp"   // protocol::IoT::client device-query encode/decode
#include "interface.hpp"
#include "turn_on_audio.hpp"        // protocol::IoT record-control encode/decode

namespace core::app {

namespace {

constexpr uint32_t kLoopDelayMs = 10;
constexpr uint32_t kKeepaliveMs = 3000;  // 3 s: >=2 round-trips of margin vs the 8 s timeout
constexpr const char* kHost = "172.30.165.156";
constexpr const char* kPort = "8080";

} // namespace

AppLoop::AppLoop(INetwork& network, IAudioService& audio, IDeviceInfo& device,
                 IFileStore& files, IFirmwareUpdater& firmware, ILink& link,
                 IClock& clock, ITaskRunner& tasks, ILogger& logger)
    : network_(network),
      audio_(audio),
      device_(device),
      files_(files),
      firmware_(firmware),
      link_(link),
      clock_(clock),
      tasks_(tasks),
      logger_(logger) {}

void AppLoop::entry(void* arg) {
    if (arg != nullptr) {
        static_cast<AppLoop*>(arg)->run();
    }
}

void AppLoop::handleIncoming(const Bytes& packet) {
    // 1. Record start/stop commands from the server.
    if (protocol::IoT::client::isRecordControl(packet)) {
        protocol::IoT::RecordControlInfo info;
        if (!protocol::IoT::client::decodeRecordControl(packet, info)) {
            logger_.logf(LogLevel::Warn, "Failed to decode valid RecordControl payload.");
            return;
        }

        if (info.command == protocol::IoT::RecordCommand::START) {
            logger_.logf(LogLevel::Info,
                         "Record START (requestId=%u, ts=%lld)",
                         static_cast<unsigned>(info.requestId),
                         static_cast<long long>(info.timestampMs));
            // Always pass requestId and always ack (including already-running).
            audio_.start(info.requestId);
        } else if (info.command == protocol::IoT::RecordCommand::STOP) {
            logger_.logf(LogLevel::Info,
                         "Record STOP (requestId=%u)",
                         static_cast<unsigned>(info.requestId));
            // Always ack (including already-stopped).
            audio_.stop(info.requestId);
        }
        return;
    }

    // 2. Device-query acknowledgement.
    if (protocol::IoT::client::isDeviceQueryResponse(packet)) {
        protocol::IoT::BuisDeviceQueryResponse response;
        if (!protocol::IoT::client::decodeDeviceQueryResponse(packet, response)) {
            logger_.logf(LogLevel::Warn, "Failed to decode BuisDeviceQueryResponse payload.");
            return;
        }
        if (response.status == protocol::IoT::Status_DeviceQuery::SUCCESS) {
            logger_.logf(LogLevel::Info, "Device query acknowledged: %s",
                         response.message.c_str());

            // The device reached the server and works: cancel OTA rollback
            // exactly once.
            if (!firmware_marked_ && firmware_.markAppValid()) {
                firmware_marked_ = true;
                logger_.logf(LogLevel::Info, "Firmware marked valid");
            }
        } else if (response.status == protocol::IoT::Status_DeviceQuery::AUTH_REQUIRED) {
            logger_.logf(LogLevel::Warn,
                         "Device query requires authentication (challenge flow not implemented yet)");
        } else {
            logger_.logf(LogLevel::Warn, "Device query rejected (status %u): %s",
                         static_cast<unsigned>(response.status), response.message.c_str());
        }
        return;
    }

    logger_.logf(LogLevel::Info, "Received general server message or unhandled packet type.");
}

void AppLoop::run() {
    if (!files_.init()) {
        logger_.logf(LogLevel::Warn, "Filesystem mount failed");
    }
    firmware_.init();
    link_.start();                 // WiFi provisioning / association
    network_.start(kHost, kPort);

    const std::string device_id = device_.deviceId();
    const std::vector<uint8_t> registration =
        protocol::IoT::client::encodeDeviceQuery(device_id);

    // Registration is sent on (re)connect transitions and then as a 3 s
    // keepalive; it is never spammed from the fast loop.
    bool registered = false;
    uint32_t last_epoch = 0;
    uint32_t last_registration_ms = 0;

    while (true) {
        // Reconnect detection via the session epoch: every reset bumps the
        // epoch, so a fast Established->reset->Established cycle is never missed.
        const uint32_t epoch = network_.epoch();
        if (epoch != last_epoch) {
            last_epoch = epoch;
            registered = false;

            // A new server session must not inherit a running capture: the
            // server has to send a fresh START command after every reconnect.
            if (audio_.isRunning()) {
                // Quiet stop: there is no session to ack against right now.
                audio_.stop(0, /*ack=*/false);
                logger_.logf(LogLevel::Info, "Audio stopped after session reset");
            }
        }

        if (network_.isEstablished() &&
            (!registered ||
             (clock_.nowMs() - last_registration_ms) >= kKeepaliveMs)) {
            network_.send(registration);
            registered = true;
            last_registration_ms = clock_.nowMs();
        }

        // Drain everything the session produced since the last tick.
        while (auto msg = network_.receive()) {
            handleIncoming(*msg);
        }

        tasks_.delayMs(kLoopDelayMs);   // yield to keep the task watchdog happy
    }

    tasks_.exitCurrentTask();           // unreachable; documents task lifecycle
}

} // namespace core::app
