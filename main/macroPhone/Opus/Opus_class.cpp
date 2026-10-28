
#include "Opus_class.hpp" // Includes the class declaration.
#include "esp_heap_caps.h"
#include <esp_log.h> // Provides ESP-IDF logging macros.

static const char* TAG = "Opus"; // Defines the logging tag.
static constexpr int MAX_PACKET_SIZE = 1000; // Sets the maximum encoded packet capacity in bytes.

// Constructs the wrapper in an uninitialized state.
Opus::Opus()
    : encoder_(nullptr), // Starts without an encoder.
      bitsPerChannel_(0), // Marks the PCM bit depth as unconfigured.
      frameSize_(0) {} // Marks the frame size as unconfigured.

// Releases resources when the object is destroyed.
Opus::~Opus() {
    release(); // Frees the encoder if it exists.
}

// Initializes the encoder and calculates the frame size.
bool Opus::init(int sampleRate, int channelCount, int bitsPerChannel,
                int targetFrameDurationMs) {
    release(); // Releases any previous encoder.

    // Checks the sampling rates supported by Opus.
    const bool validSampleRate =
        sampleRate == 8000 || sampleRate == 12000 ||
        sampleRate == 16000 || sampleRate == 24000 ||
        sampleRate == 48000;

    // This wrapper supports mono and stereo.
    const bool validChannels = channelCount == 1 || channelCount == 2;

    // Accepts signed 16-bit PCM or 32-bit floating-point PCM.
    const bool validBitDepth =
        bitsPerChannel == 16 || bitsPerChannel == 32;

    // Accepts supported Opus frame durations expressed in whole milliseconds.
    const bool validDuration =
        targetFrameDurationMs == 5 || targetFrameDurationMs == 10 ||
        targetFrameDurationMs == 20 || targetFrameDurationMs == 40 ||
        targetFrameDurationMs == 60;

    // Rejects invalid configuration before creating the encoder.
    if (!validSampleRate || !validChannels ||
        !validBitDepth || !validDuration) {
        ESP_LOGE(TAG, "Invalid Opus configuration");
        return false;
    }

    // Calculates the samples required per channel for each frame.
    frameSize_ = sampleRate * targetFrameDurationMs / 1000;

    // Initializes the Opus encoder using the audio application profile.
    int error = OPUS_OK;
    encoder_ = opus_encoder_create(
        sampleRate, channelCount, OPUS_APPLICATION_AUDIO, &error);

    // Checks whether encoder creation succeeded.
    if (error != OPUS_OK || encoder_ == nullptr) {
        ESP_LOGE(TAG, "Encoder creation failed: %s", opus_strerror(error));
        encoder_ = nullptr; // Clears the failed encoder pointer.
        frameSize_ = 0; // Resets the calculated frame size.
        return false; // Reports initialization failure.
    }

    // Saves the PCM format for subsequent encode calls.
    bitsPerChannel_ = bitsPerChannel;
channelCount_=channelCount;
    // Logs the successful initialization.
    ESP_LOGI(TAG, "Encoder initialized: %d Hz, %d channel(s), %d-bit PCM, frame size %d",
             sampleRate, channelCount, bitsPerChannel_, frameSize_);
 //TODO add complexity here;
int complexity = 3;

int err = opus_encoder_ctl(
    encoder_,
    OPUS_SET_COMPLEXITY(complexity)
);

if (err != OPUS_OK) {
    // Handle configuration error

        ESP_LOGE(TAG, "opus_encoder_ctl failed: %s", opus_strerror(error));
}


    return true; // Reports successful initialization.
}

 
std::vector<unsigned char> Opus::encode(const void* pcmData) {
    // Reject calls made before initialization or with null input.
    if (encoder_ == nullptr || pcmData == nullptr) {
        ESP_LOGE(TAG, "Encoder is uninitialized or PCM input is null");
        return {};
    }

    // Allocate space for the compressed Opus packet.
    std::vector<unsigned char> packet(MAX_PACKET_SIZE);

    int bytesWritten = 0;

    if (bitsPerChannel_ == 16) {
        // Encode signed 16-bit PCM directly.
        bytesWritten = opus_encode(
            encoder_,
            static_cast<const opus_int16*>(pcmData),
            frameSize_,
            packet.data(),
            static_cast<opus_int32>(packet.size())
        );
    } else if (bitsPerChannel_ == 32) {
        // Interpret input as signed 32-bit integer PCM.
        const auto* fixedSamples =
            static_cast<const int32_t*>(pcmData);
 
        // Opus expects float samples in approximately [-1.0, 1.0].
        const std::size_t sampleCount =
            static_cast<std::size_t>(frameSize_) *
            static_cast<std::size_t>(channelCount_);
 


        std::vector<float> floatPcm(sampleCount);

        // Convert signed 32-bit PCM to normalized floating-point PCM.
        for (std::size_t i = 0; i < sampleCount; ++i) {
            floatPcm[i] =
                static_cast<float>(fixedSamples[i]) / 2147483648.0f;
        }
// if (!heap_caps_check_integrity_addr(
//         reinterpret_cast<intptr_t>(floatPcm.data()), true)) {
//     ESP_LOGE("HEAP", "floatPcm corrupted after conversion");
//     abort();
// }
        // Encode the converted floating-point PCM.
        bytesWritten = opus_encode_float(
            encoder_,
            floatPcm.data(),
            frameSize_,
            packet.data(),
            static_cast<opus_int32>(packet.size())
        );
    } else {
        ESP_LOGE(TAG, "Unsupported PCM bit depth: %d", bitsPerChannel_);
        return {};
    }

    // Check for Opus encoding errors.
    if (bytesWritten < 0) {
        ESP_LOGE(TAG, "Encoding failed: %s", opus_strerror(bytesWritten));
        return {};
    }

    // Keep only the bytes actually written by Opus.
    packet.resize(static_cast<std::size_t>(bytesWritten));

    // ESP_LOGW(TAG, "Encoded payload size = %u bytes",
    //          static_cast<unsigned int>(packet.size()));

    return packet;
}




// Releases the encoder and resets its configuration.
void Opus::release() {
    // Destroys the encoder if it has been initialized.
    if (encoder_ != nullptr) {
        opus_encoder_destroy(encoder_); // Releases Opus resources.
        encoder_ = nullptr; // Prevents reuse of the destroyed pointer.
    }

    // Resets the saved PCM configuration.
    bitsPerChannel_ = 0;
    frameSize_ = 0;
}
