#pragma once

// Adapter: core::IFirmwareUpdater backed by FirmwareSwitch (ESP-IDF OTA).
// applyFromUrl() is the hook for HTTP(S) download + streaming into the
// alternate partition; the transport itself is not wired yet.

#include "core/ports.hpp"

#include <string>

#include "firmware_switch.hpp"

namespace adapters {

class EspFirmwareUpdater : public core::IFirmwareUpdater {
public:
    bool init() override { return firmware_.init(); }

    bool applyFromUrl(const std::string& /*url*/) override {
        // TODO: stream the image into beginOtaUpdate()/writeOtaData()/endOtaUpdate().
        return false;
    }

private:
    FirmwareSwitch firmware_;
};

} // namespace adapters
