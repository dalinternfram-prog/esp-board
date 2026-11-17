#pragma once

#include <cstdint>

namespace AudioService {

    // [CHANGE]: Fully creates and initializes the audio task and hardware on CPU Core 1
    void start();

    // [CHANGE]: Completely shuts down, releases hardware/codecs, and deletes the task
    void stop();

    // Checks whether the task is currently active/alive
    bool is_running();

} // namespace AudioService