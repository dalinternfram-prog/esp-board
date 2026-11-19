// Network module: worker task + SessionManager wiring from injected services.
// Thread-safety: every session/transport access is guarded by NetState::mutex.
// The worker holds the mutex only around session->tick() (never across the
// delay), and close_connection() never force-deletes the worker (killing a
// task that may hold the mutex would leave it locked forever).

#include "core/app/net_manager.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "core/app/session_factory.hpp"
#include "interface.hpp"

namespace Network {

namespace {

constexpr uint32_t kWorkerTickMs     = 10;
constexpr uint32_t kWorkerStackBytes = 8192;
constexpr int      kWorkerPriority   = 5;
constexpr int      kWorkerCore       = 1;
constexpr int      kWorkerExitWaitMs = 2000;

struct NetState {
    std::mutex mutex;

    std::unique_ptr<core::ITransport>     transport;
    std::unique_ptr<core::SessionManager> session;

    std::atomic<core::ITaskRunner::TaskId> handle{nullptr};
    std::atomic<bool>                      running{false};
};

NetState& net() {
    static NetState s;
    return s;
}

core::IAppServices*& services() {
    static core::IAppServices* s = nullptr;
    return s;
}

void network_worker_task(void* /*arg*/) {
    core::IAppServices& svc = *services();
    auto& st = net();

    svc.log.logf(core::LogLevel::Info, "Network worker task started");
    while (st.running.load()) {
        // No NetState::mutex here: SessionManager has its own tx/rx locks and
        // atomic state_/epoch_. Locking the whole tick() would stall send()/
        // poll() callers during DNS lookups or crypto work.
        if (st.session) {
            st.session->tick();
        }
        svc.tasks.delayMs(kWorkerTickMs);
    }

    {
        // Clear the handle under the mutex: start() publishes it under the same
        // lock, so a fast-exiting task cannot erase the handle start() is about
        // to store on the other core.
        std::lock_guard<std::mutex> lock(st.mutex);
        st.handle.store(nullptr);
    }
    svc.tasks.exitCurrentTask();
}

} // namespace

void init(const char* host, const char* port, core::IAppServices& svc) {
    services() = &svc;
    auto& st = net();

    std::lock_guard<std::mutex> lock(st.mutex);
    if (st.running.load()) {
        svc.log.logf(core::LogLevel::Warn, "Network already initialized");
        return;
    }

    st.transport = svc.transport_factory.create(
        host, port, core::app::nextConversationId(svc.random));
    if (!st.transport) {
        svc.log.logf(core::LogLevel::Error, "Failed to create transport");
        return;
    }

    st.session = core::app::makeSession(*st.transport, svc.link, svc.clock,
                                        svc.random, svc.crypto, svc.log);
    if (!st.session) {
        svc.log.logf(core::LogLevel::Error, "Failed to create session");
        st.transport.reset();
        return;
    }
    st.session->start();

    st.running.store(true);
    st.handle.store(svc.tasks.start("net_worker_io", kWorkerStackBytes,
                                    kWorkerPriority, kWorkerCore,
                                    network_worker_task, nullptr));
    if (st.handle.load() == nullptr) {
        svc.log.logf(core::LogLevel::Error, "Failed to create network worker task");
        st.running.store(false);
        st.session->stop();
        st.session.reset();
        st.transport.reset();
        return;
    }

    svc.log.logf(core::LogLevel::Info, "Network manager initialized");
}

void close_connection() {
    auto& st = net();
    core::IAppServices* svc = services();

    // 1. Ask the worker to exit WITHOUT holding the mutex.
    st.running.store(false);

    // 2. Wait for the worker to leave its loop. tick() only performs
    // non-blocking I/O and fast crypto, so it cannot block indefinitely.
    int waited_ms = 0;
    while (st.handle.load() != nullptr && waited_ms < kWorkerExitWaitMs) {
        if (svc) {
            svc->tasks.delayMs(10);
        }
        waited_ms += 10;
    }

    if (st.handle.load() != nullptr) {
        // Never force-delete here: the worker might hold NetState::mutex and
        // vTaskDelete would leave it locked forever.
        if (svc) {
            svc->log.logf(core::LogLevel::Error,
                          "Network worker did not exit; skipping session teardown");
        }
        return;
    }

    // 3. Worker is gone: teardown is now race-free.
    {
        std::lock_guard<std::mutex> lock(st.mutex);
        if (st.session) {
            st.session->stop();
            st.session.reset();
        }
        st.transport.reset();
    }

    if (svc) {
        svc->log.logf(core::LogLevel::Info, "Network connection closed");
    }
}

int send(const std::vector<uint8_t>& data) {
    auto& st = net();
    std::lock_guard<std::mutex> lock(st.mutex);
    return st.session ? st.session->send(data) : -1;
}

std::vector<uint8_t> poll_and_receive() {
    auto& st = net();
    std::lock_guard<std::mutex> lock(st.mutex);
    if (!st.session) {
        return {};
    }
    auto msg = st.session->poll();
    return msg.has_value() ? std::move(*msg) : std::vector<uint8_t>{};
}

bool is_established() {
    auto& st = net();
    std::lock_guard<std::mutex> lock(st.mutex);
    return st.session && st.session->isEstablished();
}

uint32_t session_epoch() {
    auto& st = net();
    std::lock_guard<std::mutex> lock(st.mutex);
    return st.session ? st.session->epoch() : 0;
}

} // namespace Network
