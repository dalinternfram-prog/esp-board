#include "device/device_info.hpp"

#include "adapters/device/EspDeviceInfo.hpp"

namespace device {

core::IDeviceInfo& info() {
    // Read-only singleton; function-local static => safe one-time init.
    static adapters::EspDeviceInfo instance;
    return instance;
}

} // namespace device
