// Application loop: device registration + server message dispatch.
// Everything arrives as injected ports; no hardware/root includes.

#include "core/app/prog.hpp"

#include <cctype>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "IoT_identity_token.hpp"   // protocol::IoT::client device-query encode/decode
#include "interface.hpp"
#include "turn_on_audio.hpp"        // protocol::IoT record-control encode/decode

namespace core::app {

namespace {

constexpr uint32_t kLoopDelayMs = 10;
constexpr uint32_t kKeepaliveMs = 3000;  // 3 s: >=2 round-trips of margin vs the 8 s timeout

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace

AppLoop::AppLoop(INetwork& network, IAudioService& audio, IDeviceInfo& device,
                 IFileStore& files, IFirmwareUpdater& firmware, ILink& link,
                 IClock& clock, ITaskRunner& tasks, ILogger& logger,
                 IStorage& storage, ICrypto& crypto,
                 std::string host, std::string port)
    : network_(network),
      audio_(audio),
      device_(device),
      files_(files),
      firmware_(firmware),
      link_(link),
      clock_(clock),
      tasks_(tasks),
      logger_(logger),
      storage_(storage),
      crypto_(crypto),
      host_(std::move(host)),
      port_(std::move(port)) {}

void AppLoop::entry(void* arg) {
    if (arg != nullptr) {
        static_cast<AppLoop*>(arg)->run();
    }
}

const std::string& AppLoop::effectiveDeviceId()
{
    if (!device_id_loaded_) {
        device_id_loaded_ = true;

        // Provisioning contract: NVS namespace "storage", key "device_id",
        // string value printed by provision_tool --generate.
        Bytes blob;
        if (storage_.get("device_id", blob) && !blob.empty()) {
            while (!blob.empty() &&
                   (blob.back() == 0 || blob.back() == '\n' || blob.back() == '\r')) {
                blob.pop_back();
            }
            device_id_.assign(blob.begin(), blob.end());
        }
        if (device_id_.empty()) {
            // Unprovisioned device: fall back to the MAC-derived hardware id.
            device_id_ = device_.deviceId();
        }
    }
    return device_id_;
}

bool AppLoop::ensureSecretKey()
{
    if (secret_loaded_) {
        return secret_key_.has_value() && !secret_key_->empty();
    }
    secret_loaded_ = true;

    // Provisioning contract: NVS namespace "storage", key "device_secret",
    // value = the 32 raw bytes or the 64 hex chars printed by provision_tool.
    Bytes blob;
    if (!storage_.get("device_secret", blob) || blob.empty()) {
        logger_.logf(LogLevel::Warn,
                     "No device secret in NVS (storage/device_secret); "
                     "challenge-response authentication disabled");
        return false;
    }

    Bytes raw;
    if (blob.size() == 32) {
        raw = std::move(blob);
    } else if (blob.size() == 64) {
        raw.resize(32);
        for (std::size_t i = 0; i < raw.size(); ++i) {
            const int hi = hexNibble(static_cast<char>(blob[i * 2]));
            const int lo = hexNibble(static_cast<char>(blob[i * 2 + 1]));
            if (hi < 0 || lo < 0) {
                logger_.logf(LogLevel::Error,
                             "device_secret contains non-hex characters");
                return false;
            }
            raw[i] = static_cast<uint8_t>((hi << 4) | lo);
        }
    } else {
        logger_.logf(LogLevel::Error,
                     "device_secret must be 32 raw bytes or 64 hex chars (got %u)",
                     static_cast<unsigned>(blob.size()));
        return false;
    }

    // The server stores sha256(secret) and uses it as the HMAC key, so the
    // device must derive the same key before answering a challenge.
    auto digest = crypto_.sha256(raw);
    if (!digest.has_value() || digest->size() != 32) {
        logger_.logf(LogLevel::Error, "sha256(device_secret) failed");
        return false;
    }
    secret_key_ = std::move(*digest);
    return true;
}

bool AppLoop::sendDeviceAuth(const Bytes& nonce)
{
    if (nonce.size() != 16) {
        logger_.logf(LogLevel::Error,
                     "Device challenge nonce is %u bytes (expected 16)",
                     static_cast<unsigned>(nonce.size()));
        return false;
    }
    if (!ensureSecretKey()) {
        return false;
    }

    const std::string& device_id = effectiveDeviceId();
    Bytes message = nonce;
    message.insert(message.end(), device_id.begin(), device_id.end());

    auto hmac = crypto_.hmacSha256(*secret_key_, message);
    if (!hmac.has_value() || hmac->size() != 32) {
        logger_.logf(LogLevel::Error, "HMAC-SHA256(device secret) failed");
        return false;
    }

    protocol::IoT::BuisDeviceAuthRequest request;
    request.deviceId = device_id;
    request.hmac = std::move(*hmac);
    network_.send(protocol::IoT::client::encodeDeviceAuth(request));
    logger_.logf(LogLevel::Info, "Device challenge response sent");
    return true;
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
            logger_.logf(LogLevel::Info,
                         "Device query requires authentication; answering challenge");
            if (!sendDeviceAuth(response.nonce)) {
                logger_.logf(LogLevel::Warn,
                             "Cannot answer device challenge (missing secret or crypto error)");
            }
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
    network_.start(host_.c_str(), port_.c_str());

    const std::string& device_id = effectiveDeviceId();
    const std::vector<uint8_t> registration =
        protocol::IoT::client::encodeDeviceQuery(device_id);
    logger_.logf(LogLevel::Info, "Device identity: %s", device_id.c_str());

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
