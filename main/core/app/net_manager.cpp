// Network module: worker task + SessionManager wiring from injected services.

#include "core/app/net_manager.hpp"

#include <atomic>
#include <memory>
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

struct NetState {
    std::unique_ptr<core::ITransport>      transport;
    std::unique_ptr<core::SessionManager>  session;
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
        if (st.session) {
            st.session->tick();
        }
        svc.tasks.delayMs(kWorkerTickMs);
    }

    st.handle.store(nullptr);
    svc.tasks.exitCurrentTask();
}

} // namespace

void init(const char* host, const char* port, core::IAppServices& svc) {
    services() = &svc;
    auto& st = net();

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
    if (!st.running.load() && !st.session) {
        return;
    }

    core::IAppServices* svc = services();
    st.running.store(false);

    for (int i = 0; i < 100 && st.handle.load() != nullptr; ++i) {
        if (svc) {
            svc->tasks.delayMs(10);
        }
    }
    if (st.handle.load() != nullptr) {
        if (svc) {
            svc->tasks.stop(st.handle.exchange(nullptr));
        }
    }

    if (st.session) {
        st.session->stop();
        st.session.reset();
    }
    st.transport.reset();

    if (svc) {
        svc->log.logf(core::LogLevel::Info, "Network connection closed");
    }
}

int send(const std::vector<uint8_t>& data) {
    auto& st = net();
    return st.session ? st.session->send(data) : -1;
}

std::vector<uint8_t> poll_and_receive() {
    auto& st = net();
    if (!st.session) {
        return {};
    }
    auto msg = st.session->poll();
    return msg.has_value() ? std::move(*msg) : std::vector<uint8_t>{};
}

} // namespace Network
