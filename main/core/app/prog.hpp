#pragma once

// Application loop: registration keepalive + server-message dispatch.
// A plain class with injected ports; started through ITaskRunner::start().

#include "interface.hpp"

namespace core::app {

class AppLoop {
public:
    AppLoop(INetwork& network, IAudioService& audio, IDeviceInfo& device,
            IFileStore& files, IFirmwareUpdater& firmware, ILink& link,
            IClock& clock, ITaskRunner& tasks, ILogger& logger);

    // ITaskRunner entry point; arg must be an AppLoop*.
    static void entry(void* arg);

    void run();

private:
    void handleIncoming(const Bytes& packet);

    INetwork&         network_;
    IAudioService&    audio_;
    IDeviceInfo&      device_;
    IFileStore&       files_;
    IFirmwareUpdater& firmware_;
    ILink&            link_;
    IClock&           clock_;
    ITaskRunner&      tasks_;
    ILogger&          logger_;

    bool firmware_marked_ = false;
};

} // namespace core::app
