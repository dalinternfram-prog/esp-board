#pragma once

// Hardware entry point. main.cpp includes this through HARDWARE_HEADER so the
// board/MCU implementation can be swapped without touching the application.

#include "interface.hpp"

namespace hardware {

// Brings up the board: I2S/BSP, IO expander, SD card and the RGB LED subsystem
// (queue + task). Media/UI startup is the composition root's job (it owns the
// ports); RGB_Example() only creates the hardware resources those ports use.
void boot(const core::AudioConfig& config);

} // namespace hardware

