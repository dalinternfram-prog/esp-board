// Network module: worker task + delegation to the AppContext session.
// Pure C++: scheduling and logging go through core ports.

#include "core/app/net_manager.hpp"

#include <atomic>
#include <memory>
#include <vector>

#include "app_context.hpp"
#include "interface.hpp"

namespace Network {

namespace {

constexpr uint32_t kWorkerTickMs     = 10;
constexpr uint32_t kWorkerStackBytes = 8192;
constexpr int      kWorkerPriority   = 5;
constexpr int      kWorkerCore       = 1;

struct Worker {
    std::atomic<core::ITaskRunner::TaskId> handle{nullptr};
    std::atomic<bool> running{false};
};

} // namespace

static std::unique_ptr<Worker> s_worker;

static void network_worker_task(void* arg) {
    auto* w = static_cast<Worker*>(arg);
    auto& ctx = app::context();
    ctx.logger.logf(core::LogLevel::Info, "Network worker task started");

    while (w->running.load()) {
        if (ctx.session) {
            ctx.session->tick();
        }
        ctx.tasks.delayMs(kWorkerTickMs);
    }

    w->handle.store(nullptr);
    ctx.tasks.exitCurrentTask();
}

void init(const char* host, const char* port) {
    auto& ctx = app::context();

    if (s_worker && s_worker->running.load()) {
        ctx.logger.logf(core::LogLevel::Warn, "Network already initialized");
        return;
    }

    ctx.buildSession(host, port);

    if (!s_worker) {
        s_worker = std::make_unique<Worker>();
    }
    s_worker->running.store(true);
    s_worker->handle.store(ctx.tasks.start("net_worker_io", kWorkerStackBytes,
                                           kWorkerPriority, kWorkerCore,
                                           network_worker_task, s_worker.get()));

    if (s_worker->handle.load() == nullptr) {
        ctx.logger.logf(core::LogLevel::Error, "Failed to create network worker task");
        s_worker->running.store(false);
        return;
    }

    ctx.logger.logf(core::LogLevel::Info, "Network manager initialized");
}

void close_connection() {
    auto& ctx = app::context();
    if (!s_worker) {
        return;
    }

    s_worker->running.store(false);
    for (int i = 0; i < 100 && s_worker->handle.load() != nullptr; ++i) {
        ctx.tasks.delayMs(10);
    }
    if (s_worker->handle.load() != nullptr) {
        ctx.tasks.stop(s_worker->handle.exchange(nullptr));
    }
    s_worker.reset();

    if (ctx.session) {
        ctx.session->stop();
    }
    ctx.logger.logf(core::LogLevel::Info, "Network connection closed");
}

int send(const std::vector<uint8_t>& data) {
    auto& session = app::context().session;
    return session ? session->send(data) : -1;
}

std::vector<uint8_t> poll_and_receive() {
    auto& session = app::context().session;
    if (!session) {
        return {};
    }
    auto msg = session->poll();
    return msg.has_value() ? std::move(*msg) : std::vector<uint8_t>{};
}

} // namespace Network
