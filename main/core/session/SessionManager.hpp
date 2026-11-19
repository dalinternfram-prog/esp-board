#pragma once

// Domain core: server session state machine (handshake, liveness, reconnect,
// encryption, outbound queue). Pure C++: it only knows the ports, never
// ESP-IDF / FreeRTOS / KCP / WiFi. This makes it unit-testable on the host.

#include "interface.hpp"

#include <mutex>
#include <queue>
#include <vector>

namespace core {

class SessionManager {
public:
    SessionManager(ITransport& transport, ILink& link, IClock& clock,
                   IRandom& random, ICrypto& crypto, ILogger& logger);
    ~SessionManager();

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    // Reset state and open the transport. Call once before the worker starts.
    void start();
    // Close the transport and stop the state machine.
    void stop();

    // Drive one iteration of the state machine (call every ~10 ms).
    void tick();

    // Enqueue an application message (bounded). Returns -1 on backpressure.
    int send(const Bytes& app_message);

    // Dequeue one decrypted application message, if any.
    std::optional<Bytes> poll();

    bool isEstablished() const { return state_ == State::Established; }

    // Monotonic counter bumped on every session reset. Applications use it to
    // detect (re)connection transitions even inside one tick interval.
    uint32_t epoch() const { return epoch_; }

private:
    enum class State { Idle, WaitAck, Established };

    void handleHandshake(uint32_t now_ms);
    void handleIncoming(Bytes message, uint32_t now_ms);
    void resetSession(const char* reason);
    void logf(LogLevel level, const char* fmt, ...);

    ITransport& transport_;
    ILink&      link_;
    IClock&     clock_;
    IRandom&    random_;
    ICrypto&    crypto_;
    ILogger&    logger_;

    State    state_ = State::Idle;
    uint32_t epoch_ = 0;
    Bytes    aes_key_;
    Bytes    handshake_pkt_;
    uint32_t retry_count_ = 0;
    uint32_t last_attempt_ms_ = 0;
    uint32_t waitack_start_ms_ = 0;
    uint32_t last_open_attempt_ms_ = 0;

    // Outbound double buffer: producers append to incoming_, worker swaps.
    std::mutex         tx_mutex_;
    std::vector<Bytes> incoming_;
    std::vector<Bytes> sending_;

    // Decrypted inbound messages for the application.
    std::mutex        rx_mutex_;
    std::queue<Bytes> rx_queue_;
};

} // namespace core
