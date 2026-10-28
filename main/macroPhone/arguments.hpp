#pragma once











 
#include <stdint.h>
namespace macroPhone::argument {



 
// =====================================================================
// 1. REAL / INDEPENDENT VARIABLES (Inputs)
// These are the foundational constraints set by your hardware or codec.
// =====================================================================
constexpr uint32_t sample_rate = 16000;           // Hz (Audio sampling rate)
constexpr int channel_count = 2;                  // Stereo configuration (2 channels)
//TODO don't change this,it's a mysterious,i don't know why still;change this to 32,will make the sound laggy;
constexpr int bits_per_chan = 32;                 // Bit depth (16-bit PCM audio)



constexpr uint32_t target_frame_duration_ms = 20; // Target duration (Opus strictly requires 20ms frames)

// =====================================================================
// 2. COMPUTED VARIABLES (Derived)
// These are automatically calculated at compile-time to prevent mismatches.
// =====================================================================




} ;