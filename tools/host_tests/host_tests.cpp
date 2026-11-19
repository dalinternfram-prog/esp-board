// Host unit tests for the portable core (no ESP-IDF, no hardware).
// Build/run: tools/run_host_tests.sh

#include <cstdint>
#include <cstdio>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "core/audio/AudioPipeline.hpp"
#include "core/session/SessionManager.hpp"
#include "general/input/button_state_machine.hpp"
#include "interface.hpp"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                               \
        }                                                               \
    } while (0)

// ---------------------------------------------------------------- fakes

struct FakeClock : core::IClock {
    uint32_t ms = 1000;
    uint32_t nowMs() const override { return ms; }
    int64_t nowUnixMs() const override { return static_cast<int64_t>(ms); }
    void advance(uint32_t d) { ms += d; }
};

struct FakeLogger : core::ILogger {
    void log(core::LogLevel, const char*) override {}
};

struct FakeRandom : core::IRandom {
    uint32_t values[4] = {0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u};
    int i = 0;
    uint32_t u32() override { return values[i++ % 4]; }
};

struct FakeCrypto : core::ICrypto {
    std::optional<core::Bytes> generatePublicKey() override {
        return core::Bytes{0xAA, 0xBB, 0xCC};
    }
    std::optional<core::Bytes> computeSharedSecret(const core::Bytes&) override {
        return core::Bytes(32, 0x42);
    }
    std::optional<core::Bytes> encrypt(const core::Bytes& p, const core::Bytes&) override {
        return p;
    }
    std::optional<core::Bytes> decrypt(const core::Bytes& p, const core::Bytes&) override {
        return p;
    }
};

struct FakeLink : core::ILink {
    bool up = false;
    bool start() override { up = true; return true; }
    bool isUp() const override { return up; }
};

struct FakeTransport : core::ITransport {
    bool open_ = false;
    bool dead = false;
    uint32_t conv = 0;
    uint32_t last_rx = 0;
    std::deque<core::Bytes> incoming;
    std::vector<core::Bytes> raw_sent;
    std::vector<core::Bytes> reliable_sent;
    int resets = 0;

    bool open() override { open_ = true; return true; }
    void close() override { open_ = false; }
    bool isOpen() const override { return open_; }
    bool isDead() const override { return dead; }
    void reset(uint32_t new_conv_id) override {
        ++resets;
        conv = new_conv_id;
        open_ = true;
        last_rx = 0;
        dead = false;
        incoming.clear();
    }
    void tick(uint32_t) override {}
    int sendReliable(const core::Bytes& d) override { reliable_sent.push_back(d); return 0; }
    int sendRaw(const core::Bytes& d) override { raw_sent.push_back(d); return 0; }
    std::optional<core::Bytes> receive(uint32_t now_ms) override {
        if (incoming.empty()) return std::nullopt;
        core::Bytes b = std::move(incoming.front());
        incoming.pop_front();
        last_rx = now_ms;
        return b;
    }
    uint32_t lastRxMs() const override { return last_rx; }
    uint32_t conversationId() const override { return conv; }
};

struct FakeSource : core::IAudioSource {
    int frames_left = 3;
    std::size_t bytes_per_frame = 8;
    bool open() override { return true; }
    void close() override {}
    bool readFrame(core::Bytes& pcm) override {
        if (frames_left-- <= 0) return false;
        pcm.assign(bytes_per_frame, 0x11);
        return true;
    }
};

struct FakeEncoder : core::IAudioEncoder {
    bool init() override { return true; }
    bool encode(const core::Bytes& pcm, core::Bytes& out) override {
        out.assign(pcm.begin(), pcm.end());
        return true;
    }
    uint32_t frameDurationMs() const override { return 20; }
    uint32_t sampleRate() const override { return 16000; }
    uint32_t channelCount() const override { return 2; }
};

// ---------------------------------------------------------------- tests

void test_button_state_machine() {
    using general::input::ButtonEvent;
    general::input::ButtonStateMachine b(1000);

    CHECK(b.update(false, 0) == ButtonEvent::None);
    CHECK(b.update(true, 100) == ButtonEvent::None);
    CHECK(b.update(false, 300) == ButtonEvent::ShortPress);

    // Long press fires while held, exactly once.
    CHECK(b.update(true, 1000) == ButtonEvent::None);
    CHECK(b.update(true, 2000) == ButtonEvent::LongPress);
    CHECK(b.update(true, 2100) == ButtonEvent::None);
    CHECK(b.update(false, 2200) == ButtonEvent::None);

    // Released exactly at the threshold still fires LongPress (no dropped press).
    general::input::ButtonStateMachine c(1000);
    CHECK(c.update(true, 0) == ButtonEvent::None);
    CHECK(c.update(false, 1000) == ButtonEvent::LongPress);
}

void test_audio_pipeline_timestamps() {
    FakeSource src; FakeEncoder enc; FakeClock clk; FakeLogger log;
    std::vector<uint32_t> timestamps;
    std::vector<core::Bytes> payloads;

    core::AudioPipeline p(src, enc, clk, log,
        [&](const core::Bytes& b, uint32_t ts) {
            timestamps.push_back(ts);
            payloads.push_back(b);
        });

    CHECK(p.step());
    CHECK(p.step());
    CHECK(p.step());
    CHECK(!p.step());   // source exhausted

    CHECK(timestamps.size() == 3);
    CHECK(!timestamps.empty() && timestamps[0] == 0);   // 0 starts a recording
    CHECK(timestamps.size() > 1 && timestamps[1] == 20);
    CHECK(timestamps.size() > 2 && timestamps[2] == 40);
    CHECK(!payloads.empty() && payloads[0].size() == 8);
}

void test_session_handshake_link_and_timeout() {
    FakeTransport tr;
    FakeLink link; link.up = true;
    FakeClock clk;
    FakeRandom rnd;
    FakeCrypto crypto;
    FakeLogger log;

    core::SessionManager sm(tr, link, clk, rnd, crypto, log);
    sm.start();

    // Idle -> WaitAck: [conv | pubkey] is sent raw.
    sm.tick();
    CHECK(tr.raw_sent.size() == 1);
    CHECK(!sm.isEstablished());

    // Server responds -> Established.
    tr.incoming.push_back(core::Bytes{1, 2, 3, 4});
    sm.tick();
    CHECK(sm.isEstablished());

    // Application data is encrypted and handed to sendReliable.
    CHECK(sm.send(core::Bytes{0x55, 0x66}) == 0);
    sm.tick();
    CHECK(tr.reliable_sent.size() == 1);

    // Link down while Established -> reset (epoch bumps, transport closes).
    link.up = false;
    sm.tick();
    CHECK(!sm.isEstablished());
    CHECK(sm.epoch() == 1);
    CHECK(!tr.isOpen());

    // Link back: reopen is throttled to once per second, then a fresh
    // handshake goes out and the session re-establishes.
    link.up = true;
    sm.tick();
    CHECK(!tr.isOpen());        // reopen throttle (kReopenThrottleMs)
    clk.advance(1100);
    sm.tick();
    CHECK(tr.isOpen());
    CHECK(tr.raw_sent.size() == 2);
    tr.incoming.push_back(core::Bytes{7});
    sm.tick();
    CHECK(sm.isEstablished());

    // Server silent > 8 s -> reset with a FRESH conversation id.
    const uint32_t conv_before = tr.conversationId();
    clk.advance(9000);
    sm.tick();
    CHECK(!sm.isEstablished());
    CHECK(tr.resets == 1);
    CHECK(tr.conversationId() != conv_before);
    CHECK(sm.epoch() == 2);
}

void test_waitack_timeout() {
    FakeTransport tr; FakeLink link; link.up = true;
    FakeClock clk; FakeRandom rnd; FakeCrypto crypto; FakeLogger log;
    core::SessionManager sm(tr, link, clk, rnd, crypto, log);
    sm.start();
    sm.tick();
    CHECK(tr.raw_sent.size() == 1);

    clk.advance(31000);   // > kHandshakeTimeoutMs
    sm.tick();
    CHECK(sm.epoch() == 1);
}

void test_dead_link_early_reset() {
    FakeTransport tr; FakeLink link; link.up = true;
    FakeClock clk; FakeRandom rnd; FakeCrypto crypto; FakeLogger log;
    core::SessionManager sm(tr, link, clk, rnd, crypto, log);
    sm.start();
    sm.tick();
    tr.incoming.push_back(core::Bytes{9});
    sm.tick();
    CHECK(sm.isEstablished());

    tr.dead = true;
    sm.tick();
    CHECK(sm.epoch() == 1);
    CHECK(tr.resets == 1);
}

} // namespace

int main() {
    test_button_state_machine();
    test_audio_pipeline_timestamps();
    test_session_handshake_link_and_timeout();
    test_waitack_timeout();
    test_dead_link_early_reset();

    if (g_failures == 0) {
        std::printf("ALL HOST TESTS PASSED\n");
        return 0;
    }
    std::printf("%d host test failure(s)\n", g_failures);
    return 1;
}
