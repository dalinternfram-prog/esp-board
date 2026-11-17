#include "device/device_info.hpp"

#include "app_context.hpp"

namespace device {

core::IDeviceInfo& info() { return app::context().device; }

} // namespace device
