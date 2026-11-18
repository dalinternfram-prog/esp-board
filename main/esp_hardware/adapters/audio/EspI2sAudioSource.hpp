#pragma once

// Adapter: microphone capture through the I2S hardware driver.
// The driver is created on open() so constructing the adapter (in AppContext)
// does not touch the hardware until recording actually starts.

#include "interface.hpp"

#include <memory>
#include <vector>

#include "audio_capture_driver.hpp"

namespace adapters {

class EspI2sAudioSource : public core::IAudioSource {
public:
    explicit EspI2sAudioSource(const core::AudioConfig& config) : config_(config) {}

    bool open() override {
        if (!driver_) {
            driver_ = std::make_unique<Hardware::AudioCaptureDriver>(config_);
            if (!driver_->isReady()) {
                driver_.reset();   // PSRAM allocation failed
                return false;
            }
        }
        return true;
    }

    void close() override { driver_.reset(); }

    bool readFrame(std::vector<int16_t>& pcm) override {
        if (!driver_) {
            return false;
        }
        if (driver_->readSamples() != ESP_OK) {
            return false;
        }
        pcm = driver_->getFrameBuffer();
        return !pcm.empty();
    }

private:
    core::AudioConfig config_;
    std::unique_ptr<Hardware::AudioCaptureDriver> driver_;
};

} // namespace adapters

