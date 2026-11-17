#pragma once

// Hardware entry point. main.cpp includes this through HARDWARE_HEADER so the
// board/MCU implementation can be swapped without touching the application.

namespace hardware {

// Brings up the board: I2S/BSP, IO expander, SD card, audio playback, buttons,
// speech engine and the music library scan.
void boot();

} // namespace hardware
