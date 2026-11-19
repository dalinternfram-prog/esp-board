#pragma once

// Application loop: registration keepalive + server-message dispatch.
// A plain class with injected ports; started through ITaskRunner::start().

#include "interface.hpp"

#include <optional>
#include <string>

namespace core::app {

class AppLoop {
public:
    AppLoop(INetwork& network, IAudioService& audio, IDeviceInfo& device,
            IFileStore& files, IFirmwareUpdater& firmware, ILink& link,
            IClock& clock, ITaskRunner& tasks, ILogger& logger,
            IStorage& storage, ICrypto& crypto,
            std::string host, std::string port);

    // ITaskRunner entry point; arg must be an AppLoop*.
    static void entry(void* arg);

    void run();

private:
    void handleIncoming(const Bytes& packet);

    // Loads the provisioned secret once and derives sha256(secret), the key
    // the server uses for HMAC verification.
    bool ensureSecretKey();
    // Answers an AUTH_REQUIRED challenge with HMAC-SHA256(nonce || deviceId).
    bool sendDeviceAuth(const Bytes& nonce);
    // Provisioned identity from NVS (storage/device_id), falling back to the
    // MAC-derived hardware id. The sticker id and the server row must match it.
    const std::string& effectiveDeviceId();

    INetwork&         network_;
    IAudioService&    audio_;
    IDeviceInfo&      device_;
    IFileStore&       files_;
    IFirmwareUpdater& firmware_;
    ILink&            link_;
    IClock&           clock_;
    ITaskRunner&      tasks_;
    ILogger&          logger_;
    IStorage&         storage_;
    ICrypto&          crypto_;
    std::string       host_;
    std::string       port_;

    bool firmware_marked_ = false;
    bool secret_loaded_ = false;
    std::optional<Bytes> secret_key_;   // sha256(raw device secret)
    bool device_id_loaded_ = false;
    std::string device_id_;
};

} // namespace core::app
