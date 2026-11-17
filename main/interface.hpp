#pragma once

// Platform-agnostic ports (interfaces) for the application core.
// IMPORTANT: no ESP-IDF / FreeRTOS / KCP / WiFi headers may be included here.
// Implementations live in main/esp_hardware/* and are wired in the composition
// root (main/esp_hardware/app_context.hpp).

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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
    virtual bool readFrame(std::vector<int16_t>& pcm) = 0;
};

// Audio codec (Opus / LC3 / ...).
struct IAudioEncoder {
    virtual ~IAudioEncoder() = default;
    virtual bool init() = 0;
    virtual bool encode(const std::vector<int16_t>& pcm, Bytes& out) = 0;
    virtual uint32_t frameDurationMs() const = 0;
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

    // Rebuild the transport protocol state so the peer restarts from a clean
    // sequence (e.g. after a server restart). Must be safe to call repeatedly.
    virtual void reset() = 0;

    // Drive internal timers / retransmissions (called every worker tick).
    virtual void tick(uint32_t now_ms) = 0;

    // Reliable, ordered delivery (application data).
    virtual int sendReliable(const Bytes& data) = 0;
    // Unreliable delivery used for the handshake (must bypass reliability).
    virtual int sendRaw(const Bytes& data) = 0;

    // Return one complete inbound message, or nullopt when nothing is ready.
    virtual std::optional<Bytes> receive() = 0;

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
};

// Task/thread creation (FreeRTOS today; std::thread or another RTOS elsewhere).
struct ITaskRunner {
    using TaskId = void*;
    virtual ~ITaskRunner() = default;
    virtual TaskId start(const char* name, uint32_t stack_bytes, int priority, int core,
                         void (*fn)(void*), void* arg) = 0;
    virtual void stop(TaskId task) = 0;    // force-terminate
    virtual void delayMs(uint32_t ms) = 0;
    virtual void exitCurrentTask() = 0;    // must be called by fn() before it returns
};

// Firmware update (OTA / SD card / serial, ...).
struct IFirmwareUpdater {
    virtual ~IFirmwareUpdater() = default;
    virtual bool init() = 0;
    virtual bool applyFromUrl(const std::string& url) = 0;
};

} // namespace core
