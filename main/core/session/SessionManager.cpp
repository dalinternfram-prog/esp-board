#include "core/session/SessionManager.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <utility>

namespace core {
namespace {

constexpr uint32_t kHandshakeBaseRetryMs = 600;   // first handshake retry delay
constexpr uint32_t kHandshakeMaxRetryMs  = 3000;  // handshake retry ceiling
constexpr uint32_t kServerTimeoutMs      = 8000;  // no RX at all => server dead
constexpr uint32_t kHandshakeTimeoutMs   = 30000; // WaitAck ceiling -> fresh keypair
constexpr uint32_t kReopenThrottleMs     = 1000;  // retry transport.open() slowly
constexpr size_t   kMaxOutboundQueue     = 128;   // backpressure threshold

} // namespace

SessionManager::SessionManager(ITransport& transport, ILink& link, IClock& clock,
                               IRandom& random, ICrypto& crypto, ILogger& logger)
    : transport_(transport),
      link_(link),
      clock_(clock),
      random_(random),
      crypto_(crypto),
      logger_(logger) {}

SessionManager::~SessionManager() { stop(); }

void SessionManager::logf(LogLevel level, const char* fmt, ...) {
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    logger_.log(level, buf);
}

void SessionManager::start() {
    state_.store(State::Idle);
    epoch_.store(0);
    aes_key_.clear();
    handshake_pkt_.clear();
    retry_count_ = 0;
    last_attempt_ms_ = 0;
    waitack_start_ms_ = 0;
    last_open_attempt_ms_ = 0;

    {
        std::lock_guard<std::mutex> lk(tx_mutex_);
        incoming_.clear();
        sending_.clear();
    }
    {
        std::lock_guard<std::mutex> lk(rx_mutex_);
        std::queue<Bytes> empty;
        std::swap(rx_queue_, empty);
    }

    if (link_.isUp() && !transport_.open()) {
        logf(LogLevel::Error, "Transport open failed; will keep retrying");
    }
}

void SessionManager::stop() {
    transport_.close();
    state_.store(State::Idle);
}

int SessionManager::send(const Bytes& app_message) {
    if (app_message.empty()) {
        return 0;
    }
    std::lock_guard<std::mutex> lk(tx_mutex_);
    if (incoming_.size() >= kMaxOutboundQueue) {
        return -1; // backpressure: caller should drop instead of buffering
    }
    incoming_.push_back(app_message);
    return 0;
}

std::optional<Bytes> SessionManager::poll() {
    std::lock_guard<std::mutex> lk(rx_mutex_);
    if (rx_queue_.empty()) {
        return std::nullopt;
    }
    Bytes out = std::move(rx_queue_.front());
    rx_queue_.pop();
    return out;
}

void SessionManager::tick() {
    const uint32_t now = clock_.nowMs();

    // 0. Ensure the transport is open (retry, but throttled). Only attempt
    // when the link is up: if kHost is a hostname, getaddrinfo() must not
    // stall this worker while WiFi is down.
    if (!transport_.isOpen() && link_.isUp()) {
        if (last_open_attempt_ms_ == 0 || (now - last_open_attempt_ms_) >= kReopenThrottleMs) {
            last_open_attempt_ms_ = now;
            if (transport_.open()) {
                logf(LogLevel::Info, "Transport opened");
            }
        }
    }
    bool transport_ok = transport_.isOpen();

    if (transport_ok) {
        // 1. Drive transport timers (retransmissions, acks, ...).
        transport_.tick(now);

        // 1b. KCP dead_link: retransmission limit exhausted -> reconnect early
        // instead of waiting for the full server-timeout window.
        if (state_.load() != State::Idle && transport_.isDead()) {
            resetSession("dead link");
            transport_ok = transport_.isOpen();   // reopen may have failed
        }

        if (transport_ok) {
            // 2. Handshake / reconnect state machine.
            handleHandshake(now);

            // 3. Inbound processing: drain ALL reassembled messages this tick
            // so bursts cannot pile up between ticks.
            while (auto msg = transport_.receive(now)) {
                handleIncoming(std::move(*msg), now);
            }
        }

        // 4. Server liveness: no datagram at all for kServerTimeoutMs.
        if (state_.load() == State::Established) {
            const uint32_t last_rx = transport_.lastRxMs();
            // int32 comparison is wraparound-safe (49.7-day uint32 rollover).
            if (last_rx != 0 &&
                static_cast<int32_t>(now - last_rx) > static_cast<int32_t>(kServerTimeoutMs)) {
                logf(LogLevel::Warn, "Server silent for %u ms; reconnecting",
                     static_cast<unsigned>(now - last_rx));
                resetSession("server timeout");
            }
        }
    }

    // 5. Swap producer buffer into the worker buffer.
    {
        std::lock_guard<std::mutex> lk(tx_mutex_);
        if (!incoming_.empty()) {
            sending_.swap(incoming_);
        }
    }

    // 6. Encrypt & transmit (or drop when offline).
    if (!sending_.empty()) {
        const bool online = transport_ok && link_.isUp() && state_.load() == State::Established;
        if (online) {
            for (const auto& pkt : sending_) {
                auto encrypted = crypto_.encrypt(pkt, aes_key_);
                if (!encrypted.has_value()) {
                    logf(LogLevel::Error, "Encrypt failed; dropping packet");
                    continue;
                }
                if (transport_.sendReliable(*encrypted) < 0) {
                    logf(LogLevel::Warn, "Transport send saturated; dropping packet");
                }
            }
        } else {
            logf(LogLevel::Warn, "Dropped %u packet(s): session not established",
                 static_cast<unsigned>(sending_.size()));
        }
        sending_.clear();
    }
}

void SessionManager::handleHandshake(uint32_t now_ms) {
    if (!link_.isUp()) {
        if (state_.load() != State::Idle) {
            resetSession("link down");
        }
        return;
    }

    switch (state_.load()) {
    case State::Idle: {
        // P-256 keygen is expensive and sendRaw() fails fast (ICMP
        // unreachable) while the server is down; retry at the handshake
        // interval instead of every 10 ms tick.
        if (last_attempt_ms_ != 0 && (now_ms - last_attempt_ms_) < kHandshakeBaseRetryMs) {
            return;
        }
        auto pub = crypto_.generatePublicKey();
        if (!pub.has_value() || pub->empty()) {
            logf(LogLevel::Error, "ECDH key generation failed");
            return;
        }
        const uint32_t conv = transport_.conversationId();
        handshake_pkt_.resize(sizeof(uint32_t) + pub->size());
        std::memcpy(handshake_pkt_.data(), &conv, sizeof(uint32_t));
        std::memcpy(handshake_pkt_.data() + sizeof(uint32_t), pub->data(), pub->size());
        if (transport_.sendRaw(handshake_pkt_) < 0) {
            logf(LogLevel::Warn, "Handshake send failed; staying in Idle");
            last_attempt_ms_ = now_ms;   // throttle the next key generation
            return;
        }
        last_attempt_ms_ = now_ms;
        retry_count_ = 1;
        state_.store(State::WaitAck);
        waitack_start_ms_ = now_ms;
        logf(LogLevel::Info, "Handshake dispatched (conv=%u)", static_cast<unsigned>(conv));
        break;
    }
    case State::WaitAck: {
        if ((now_ms - waitack_start_ms_) > kHandshakeTimeoutMs) {
            logf(LogLevel::Warn, "Handshake timed out; restarting with a fresh keypair");
            resetSession("handshake timeout");
            return;
        }
        uint32_t interval = retry_count_ * kHandshakeBaseRetryMs;
        if (interval > kHandshakeMaxRetryMs) {
            interval = kHandshakeMaxRetryMs;
        }
        if ((now_ms - last_attempt_ms_) >= interval) {
            retry_count_++;
            transport_.sendRaw(handshake_pkt_);
            last_attempt_ms_ = now_ms;
            logf(LogLevel::Warn, "Handshake ACK timeout; retry #%u",
                 static_cast<unsigned>(retry_count_));
        }
        break;
    }
    case State::Established:
        break; // liveness handled in tick()
    }
}

void SessionManager::handleIncoming(Bytes message, uint32_t now_ms) {
    (void)now_ms;
    if (state_.load() == State::WaitAck) {
        auto secret = crypto_.computeSharedSecret(message);
        if (secret.has_value() && !secret->empty()) {
            aes_key_ = std::move(*secret);
            handshake_pkt_.clear();
            retry_count_ = 0;
            state_.store(State::Established);
            logf(LogLevel::Info, "Handshake complete; AES-256-GCM key established");
        } else {
            logf(LogLevel::Error, "Corrupt handshake response");
        }
        return;
    }
    if (state_.load() == State::Established) {
        auto decrypted = crypto_.decrypt(message, aes_key_);
        if (!decrypted.has_value()) {
            logf(LogLevel::Error, "Failed to decrypt incoming packet");
            return;
        }
        std::lock_guard<std::mutex> lk(rx_mutex_);
        rx_queue_.push(std::move(*decrypted));
    }
}

void SessionManager::resetSession(const char* reason) {
    logf(LogLevel::Warn, "Session reset (%s)", reason);
    state_.store(State::Idle);
    aes_key_.clear();
    handshake_pkt_.clear();
    retry_count_ = 0;
    last_attempt_ms_ = 0;
    waitack_start_ms_ = 0;
    if (link_.isUp()) {
        // Recreate the transport with a FRESH conversation id: reusing the old
        // conv would desync KCP sequence numbers if the peer is still alive.
        transport_.reset(random_.u32());
    } else {
        // Link is down: no socket/DNS work now; tick() reopens once isUp().
        transport_.close();
    }
    epoch_.fetch_add(1);
    last_open_attempt_ms_ = clock_.nowMs();
}

} // namespace core
