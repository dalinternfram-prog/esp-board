
#pragma once // Prevents multiple inclusions of this header.

#include <opus.h> // Provides the Opus encoder API.
#include <cstdint> // Provides fixed-width integer types.
#include <vector> 
class Opus { // Defines a minimal Opus encoder wrapper.
public: // Exposes the encoder interface.

    Opus(); // Initializes the encoder pointer and configuration.
    ~Opus(); // Releases the encoder automatically.

    Opus(const Opus&) = delete; // Prevents copying the encoder pointer.
    Opus& operator=(const Opus&) = delete; // Prevents copy assignment.

    // Initializes the encoder using the four core configuration arguments.
    bool init(int sampleRate, int channelCount, int bitsPerChannel,
              int targetFrameDurationMs);

    // Encodes one PCM frame and returns the compressed packet size in bytes.
    // Returns a negative Opus error code if encoding fails.
    std::vector<unsigned char> encode(const void* pcmData);

    // Releases the encoder when it is no longer needed.
    void release();

private: // Keeps encoder implementation details private.

    OpusEncoder* encoder_; // Owns the Opus encoder instance.
    int channelCount_; // Stores the configured channel count.
    int bitsPerChannel_; // Stores the PCM input bit depth.
    int frameSize_; // Stores samples per channel for each frame.
};
