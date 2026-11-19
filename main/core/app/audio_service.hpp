#pragma once

#include <cstdint>

namespace core { struct IAppServices; }

namespace AudioService {

    // Starts the capture/encode task. The START ack is sent only after the
    // hardware/codec are actually open (ok=false on failure).
    void start(core::IAppServices& services, uint32_t request_id = 0);

    // Stops the task and releases the audio hardware. When ack is false the
    // STOP is silent (used for reconnect cleanup, where no session exists).
    void stop(core::IAppServices& services, uint32_t request_id = 0, bool ack = true);

    bool is_running();

} // namespace AudioService
