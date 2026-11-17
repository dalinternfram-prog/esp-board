// Application entry point: board bring-up + application task.
// Everything hardware-specific lives behind HARDWARE_HEADER; swap the macro to
// build the same core/ on another MCU.

#ifndef HARDWARE_HEADER
#define HARDWARE_HEADER "esp_hardware/hardware.hpp"
#endif
#include HARDWARE_HEADER

#include "app_context.hpp"
#include "core/app/prog.hpp"

extern "C" void app_main() {
    hardware::boot();

    if (app::context().tasks.start("app_run_loop_task", 9152, 5, -1,
                                   app_run_loop, nullptr) == nullptr) {
        app::context().logger.logf(core::LogLevel::Error,
                                   "Failed to start the application task");
    }
}
