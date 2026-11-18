#pragma once

// Hardware entry point. main.cpp includes this through HARDWARE_HEADER so the
// board/MCU implementation can be swapped without touching the application.

#include "interface.hpp"

namespace hardware {

// Brings up the board: I2S/BSP, IO expander and SD card.
// Media/UI startup is the composition root's job (it owns the ports).
void boot(const core::AudioConfig& config);

} // namespace hardware

