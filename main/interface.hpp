#pragma once

// Platform-agnostic ports (interfaces) for the application core.
// IMPORTANT: no ESP-IDF / FreeRTOS / KCP / WiFi headers may be included here.
// Implementations live in main/esp_hardware/* and are wired in the composition
// root (main/app_context.hpp).

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace core {

using Bytes = std::vector<uint8_t>;

enum class LogLevel { Info, Warn, Error };

// Logging sink (ESP_LOGx today, stdout/other tomorrow).
struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, const char* msg) = 0;

    // Convenience printf-style helper (non-virtual). Adapters implement log().
    void logf(LogLevel level, const char* fmt, ...) {
        char buf[256];
        va_list ap;
        va_start(ap, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        log(level, buf);
    }
};

// Monotonic millisecond clock.
struct IClock {
    virtual ~IClock() = default;
    virtual uint32_t nowMs() const = 0;
    virtual int64_t  nowUnixMs() const = 0;
};

// Randomness source (used for the KCP conversation id).
struct IRandom {
    virtual ~IRandom() = default;
    virtual uint32_t u32() = 0;
};

// Connectivity: "is there a usable path to the server?".
// WiFi today; 4G / Ethernet tomorrow without touching the core.
struct ILink {
    virtual ~ILink() = default;
    virtual bool start() = 0;       // begin provisioning/connect (WiFi portal, PPP dial)
    virtual bool isUp() const = 0;  // usable path (associated + IP)
};

// Hardware identity, sealed away from the application: the app never calls
// esp_read_mac() / esp_chip_info() directly.
struct IDeviceInfo {
    virtual ~IDeviceInfo() = default;
    virtual std::string deviceId() const = 0;
    virtual std::string mac() const = 0;
    virtual std::string chip() const = 0;
};

// Audio capture (microphone / I2S / ...). Fills one PCM frame per call.
struct IAudioSource {
    virtual ~IAudioSource() = default;
    virtual bool open() = 0;
    virtual void close() = 0;
    // Raw bytes of one PCM frame in the encoder's input format
    // (AudioConfig::pcm_bits_per_chan). The capture driver has already
    // unpacked the board's I2S slot layout into this format.
    virtual bool readFrame(Bytes& pcm) = 0;
};

// Audio codec (Opus / LC3 / ...).
struct IAudioEncoder {
    virtual ~IAudioEncoder() = default;
    virtual bool init() = 0;
    virtual bool encode(const Bytes& pcm, Bytes& out) = 0;   // raw PCM bytes
    virtual uint32_t frameDurationMs() const = 0;
    virtual uint32_t sampleRate() const = 0;
    virtual uint32_t channelCount() const = 0;
};

// Persistence: small blobs (keys, credentials, counters). NVS today, any
// other key-value store tomorrow.
struct IStorage {
    virtual ~IStorage() = default;
    virtual bool set(const std::string& key, const Bytes& value) = 0;
    virtual bool get(const std::string& key, Bytes& out) = 0;
    virtual bool remove(const std::string& key) = 0;
    virtual bool exists(const std::string& key) = 0;
};

// File persistence (recordings, offline queues, ...). LittleFS today.
struct IFileStore {
    virtual ~IFileStore() = default;
    virtual bool init() = 0;   // mount / prepare the backing store
    virtual bool exists(const std::string& path) = 0;
    virtual bool remove(const std::string& path) = 0;
    virtual bool readAll(const std::string& path, Bytes& out) = 0;
    virtual bool writeAll(const std::string& path, const Bytes& data) = 0;
};

// Byte transport to the server. Owns the conversation id (used by the
// handshake packet) and hides whether it is UDP+KCP, PPP, TCP, ...
struct ITransport {
    virtual ~ITransport() = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // Rebuild the transport protocol state with a FRESH conversation id so the
    // peer restarts from a clean sequence (reusing the conv would desync KCP).
    virtual void reset(uint32_t new_conv_id) = 0;

    // KCP dead_link: retransmission limit exhausted. The session may reconnect
    // early instead of waiting for the full server-timeout window.
    virtual bool isDead() const { return false; }

    // Drive internal timers / retransmissions (called every worker tick).
    virtual void tick(uint32_t now_ms) = 0;

    // Reliable, ordered delivery (application data).
    virtual int sendReliable(const Bytes& data) = 0;
    // Unreliable delivery used for the handshake (must bypass reliability).
    virtual int sendRaw(const Bytes& data) = 0;

    // Return one complete inbound message, or nullopt when nothing is ready.
    virtual std::optional<Bytes> receive(uint32_t now_ms) = 0;

    // Timestamp of the last received datagram (including transport-level acks),
    // used for server liveness detection. 0 = nothing received yet.
    virtual uint32_t lastRxMs() const = 0;

    // Conversation id shared with the peer.
    virtual uint32_t conversationId() const = 0;
};

// ECDH + authenticated encryption.
struct ICrypto {
    virtual ~ICrypto() = default;
    virtual std::optional<Bytes> generatePublicKey() = 0;
    virtual std::optional<Bytes> computeSharedSecret(const Bytes& peer_public_key) = 0;
    virtual std::optional<Bytes> encrypt(const Bytes& plaintext, const Bytes& key) = 0;
    virtual std::optional<Bytes> decrypt(const Bytes& packet, const Bytes& key) = 0;
    // Device challenge-response primitives. SHA-256 derives the HMAC key from
    // the provisioned secret; HMAC-SHA256(key, nonce || deviceId) proves it.
    virtual std::optional<Bytes> sha256(const Bytes& data) = 0;
    virtual std::optional<Bytes> hmacSha256(const Bytes& key, const Bytes& data) = 0;
};

// Task/thread creation (FreeRTOS today; std::thread or another RTOS elsewhere).
struct ITaskRunner {
    using TaskId = void*;
    virtual ~ITaskRunner() = default;
    virtual TaskId start(const char* name, uint32_t stack_bytes, int priority, int core,
                         void (*fn)(void*), void* arg) = 0;   // core < 0 = no affinity
    virtual void stop(TaskId task) = 0;    // force-terminate
    virtual void delayMs(uint32_t ms) = 0;
    virtual void exitCurrentTask() = 0;    // must be called by fn() before it returns
};

// Firmware update (OTA / SD card / serial, ...).
struct IFirmwareUpdater {
    virtual ~IFirmwareUpdater() = default;
    virtual bool init() = 0;
    virtual bool applyFromUrl(const std::string& url) = 0;
    virtual bool markAppValid() = 0;   // cancel OTA rollback once healthy
};

// Media playback (music player / announcements).
enum class MediaState { None, Running, Paused, Stopped, Finished, Error };

struct IMediaPlayer {
    virtual ~IMediaPlayer() = default;
    virtual bool init() = 0;
    virtual bool play(const std::string& url) = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual MediaState state() const = 0;
    virtual uint8_t volume() const = 0;
    virtual void setVolume(uint8_t volume) = 0;
};

// Status indication (RGB LED, display, ...).
enum class IndicatorMode { Idle, Playing, Recording, VoiceCommand };
enum class IndicatorColor { Red, Green, Blue, White };

struct IIndicator {
    virtual ~IIndicator() = default;
    virtual void setMode(IndicatorMode mode) = 0;
    virtual void setColor(IndicatorColor color) = 0;
};

// Physical buttons.
enum class ButtonId { Previous, PlayPause, Next };
enum class ButtonEvent { ShortPress, LongPress };

struct IButtonInput {
    using Handler = void (*)(ButtonId id, ButtonEvent event, void* ctx);
    virtual ~IButtonInput() = default;
    virtual bool start(Handler handler, void* ctx) = 0;
};

// Voice activity / commands.
enum class SpeechEvent { Awake, Command, CommandTimeout };

struct ISpeechInput {
    using Handler = void (*)(SpeechEvent event, int command, void* ctx);
    virtual ~ISpeechInput() = default;
    virtual bool start(Handler handler, void* ctx) = 0;
};

// File listing for playback.
struct IPlaylist {
    virtual ~IPlaylist() = default;
    virtual std::vector<std::string> scan(const std::string& directory,
                                          const std::string& extension) = 0;
};

// Audio configuration shared by the application and the hardware layer.
// Defaults match the current board (see the old macroPhone::argument values).
struct AudioConfig {
    uint32_t sample_rate = 16000;
    uint32_t channel_count = 2;
    uint32_t bits_per_chan = 32;              // I2S slot width (2 x 32-bit)
    // Width of the PCM handed to the encoder. The capture driver unpacks the
    // board's 32-bit slots into this format; Opus must be configured with it.
    uint32_t pcm_bits_per_chan = 32;
    uint32_t target_frame_duration_ms = 20;   // Opus requires 20 ms
};

// Creates the transport for a session. Implemented by the composition root
// because the choice of transport is hardware-specific.
struct ITransportFactory {
    virtual ~ITransportFactory() = default;
    virtual std::unique_ptr<ITransport> create(const char* host, const char* port,
                                               uint32_t conv_id) = 0;
};

// Outbound application channel of the session (core::app::NetworkService).
// Thread-safe; send() returns <0 under backpressure or when no session is up.
struct INetwork {
    virtual ~INetwork() = default;
    // Brings the transport/session up (called once by the application loop
    // after FS/OTA/WiFi startup). Safe to call again: it no-ops if running.
    virtual void start(const char* host, const char* port) = 0;
    virtual int send(const Bytes& data) = 0;
    virtual std::optional<Bytes> receive() = 0;   // one message, if any
    virtual bool isEstablished() const = 0;
    virtual uint32_t epoch() const = 0;           // bumps on every reset
};

// Capture-session control (core::app::AudioService).
struct IAudioService {
    virtual ~IAudioService() = default;
    virtual void start(uint32_t request_id = 0) = 0;
    virtual void stop(uint32_t request_id = 0, bool ack = true) = 0;
    virtual bool isRunning() const = 0;
};

} // namespace core
