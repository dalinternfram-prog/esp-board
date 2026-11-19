#pragma once

#include <cstdint>

namespace core { struct IAppServices; }

namespace AudioService {

    // Starts the capture/encode task. The START ack is sent only after the
    // hardware/codec are actually open (ok=false on failure).
    void start(core::IAppServices& services, uint32_t request_id = 0);

    // Stops the task and releases the audio hardware, then acks the request.
    void stop(uint32_t request_id = 0);

    bool is_running();

} // namespace AudioService
