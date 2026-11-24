// Application entry point: board bring-up + application task.
// Everything hardware-specific lives behind HARDWARE_HEADER; swap the macro to
// build the same core/ on another MCU.

#ifndef HARDWARE_HEADER
#define HARDWARE_HEADER "esp_hardware/hardware.hpp"
#endif
#include HARDWARE_HEADER

#include "app_context.hpp"
#include "core/app/prog.hpp"
#include "esp_hardware/adapters/storage/EspNvsDump.hpp"

extern "C" void app_main() {
    // Program-lifetime composition root: a plain object, no singleton accessor.
    static app::AppContext context;

    // Boot-time diagnostic: print every NVS key/value once (secrets redacted).
    adapters::dumpNvsContents();

    hardware::boot(context.audio_config);
    context.startMedia();

    if (context.tasks.start("app_run_loop_task", 9152, 5, -1,
                            core::app::AppLoop::entry, &context.loop) == nullptr) {
        context.logger.logf(core::LogLevel::Error,
                            "Failed to start the application task");
    }
}
