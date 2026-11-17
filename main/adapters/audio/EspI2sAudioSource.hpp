#pragma once

// Adapter: microphone capture through the I2S hardware driver.

#include "core/ports.hpp"

#include <vector>

#include "audio_capture_driver.hpp"

namespace adapters {

class EspI2sAudioSource : public core::IAudioSource {
public:
    bool open() override { return true; }   // the driver initializes in its ctor
    void close() override {}

    bool readFrame(std::vector<int16_t>& pcm) override {
        if (driver_.readSamples() != ESP_OK) {
            return false;
        }
        pcm = driver_.getFrameBuffer();
        return !pcm.empty();
    }

private:
    Hardware::AudioCaptureDriver driver_;
};

} // namespace adapters
