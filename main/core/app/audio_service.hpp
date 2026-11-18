#pragma once

#include <cstdint>

namespace core { struct IAppServices; }

namespace AudioService {

    // Starts the capture/encode task using the injected application services.
    void start(core::IAppServices& services);

    // Stops the task and releases the audio hardware.
    void stop();

    bool is_running();

} // namespace AudioService
