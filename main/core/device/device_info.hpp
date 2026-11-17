#pragma once

// Convenience accessor for the platform device identity.
// (Phase 5 will fold this into the AppContext.)

#include "interface.hpp"

namespace device {

core::IDeviceInfo& info();

} // namespace device
