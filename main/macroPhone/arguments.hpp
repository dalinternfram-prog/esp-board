#pragma once











 
#include <stdint.h>
namespace macroPhone::argument {




//   constexpr uint32_t kDefaultChunkSize = 512;
// constexpr uint32_t sample_rate= 16000;
// constexpr int channel_count = 2;
// constexpr int bits_per_chan=16;

// =====================================================================
// 1. REAL / INDEPENDENT VARIABLES (Inputs)
// These are the foundational constraints set by your hardware or codec.
// =====================================================================
constexpr uint32_t sample_rate = 16000;           // Hz (Audio sampling rate)
constexpr int channel_count = 2;                  // Stereo configuration (2 channels)
constexpr int bits_per_chan = 16;                 // Bit depth (16-bit PCM audio)



constexpr uint32_t target_frame_duration_ms = 20; // Target duration (Opus strictly requires 20ms frames)

// =====================================================================
// 2. COMPUTED VARIABLES (Derived)
// These are automatically calculated at compile-time to prevent mismatches.
// =====================================================================

// Samples per channel per read (e.g., 16000 Hz * 0.020 sec = 320 samples)
constexpr uint32_t kDefaultChunkSize = (sample_rate * target_frame_duration_ms) / 1000;

// Total buffer size in bytes for I2S read (320 samples * 2 channels * 2 bytes = 1280 bytes)
constexpr size_t kDefaultChunkBytes = kDefaultChunkSize * channel_count * (bits_per_chan / 8);




} ;