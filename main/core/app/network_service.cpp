// NetworkService: transport + SessionManager + worker task, as an object.
// Thread-safety: session/transport pointers are guarded by mutex_; the worker
// calls SessionManager::tick() without that lock (SessionManager has its own
// tx/rx locks and atomic state/epoch). close() never force-deletes the worker:
// a task deleted while holding a lock would leave it locked forever.

#include "core/app/network_service.hpp"

#include "core/app/session_factory.hpp"

namespace core::app {

namespace {

constexpr uint32_t kWorkerTickMs     = 10;
// 8 KB was too small for the KCP flush -> lwIP netconn/socket send path:
// the first audio packet overflowed this task's stack and corrupted its
// pthread TLS, faulting in pthread_getspecific(). 16 KB keeps the stack in
// internal RAM (the 32 KB largest-block limit here is tighter) while giving
// the send path headroom.
constexpr uint32_t kWorkerStackBytes = 16384;
constexpr int      kWorkerPriority   = 5;
constexpr int      kWorkerCore       = 1;
constexpr int      kWorkerExitWaitMs = 2000;

} // namespace

NetworkService::NetworkService(ITransportFactory& factory, ILink& link,
                               IClock& clock, IRandom& random, ICrypto& crypto,
                               ITaskRunner& tasks, ILogger& logger)
    : factory_(factory),
      link_(link),
      clock_(clock),
      random_(random),
      crypto_(crypto),
      tasks_(tasks),
      logger_(logger) {}

NetworkService::~NetworkService() {
    // The owner must call close() while the task runner is still alive; here we
    // only signal the loop, never wait or touch task-runner state.
    running_.store(false);
}

void NetworkService::workerEntry(void* arg) {
    static_cast<NetworkService*>(arg)->workerLoop();
}

void NetworkService::workerLoop() {
    logger_.logf(LogLevel::Info, "Network worker task started");
    while (running_.load()) {
        // No mutex_ here: SessionManager is internally synchronized and a tick
        // may perform DNS/crypto work that must not stall send()/receive().
        if (session_) {
            session_->tick();
        }
        tasks_.delayMs(kWorkerTickMs);
    }

    {
        // Clear the handle under the mutex: start() publishes it under the same
        // lock, so a fast-exiting task cannot erase the handle start() is about
        // to store on the other core.
        std::lock_guard<std::mutex> lock(mutex_);
        handle_.store(nullptr);
    }
    tasks_.exitCurrentTask();
}

void NetworkService::start(const char* host, const char* port) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_.load()) {
        logger_.logf(LogLevel::Warn, "Network already initialized");
        return;
    }

    transport_ = factory_.create(host, port, nextConversationId(random_));
    if (!transport_) {
        logger_.logf(LogLevel::Error, "Failed to create transport");
        return;
    }

    session_ = makeSession(*transport_, link_, clock_, random_, crypto_, logger_);
    if (!session_) {
        logger_.logf(LogLevel::Error, "Failed to create session");
        transport_.reset();
        return;
    }
    session_->start();

    running_.store(true);
    handle_.store(tasks_.start("net_worker_io", kWorkerStackBytes, kWorkerPriority,
                               kWorkerCore, workerEntry, this));
    if (handle_.load() == nullptr) {
        logger_.logf(LogLevel::Error, "Failed to create network worker task");
        running_.store(false);
        session_->stop();
        session_.reset();
        transport_.reset();
        return;
    }

    logger_.logf(LogLevel::Info, "Network manager initialized");
}

void NetworkService::close() {
    // 1. Ask the worker to exit WITHOUT holding the mutex.
    running_.store(false);

    // 2. Wait for the worker to leave its loop. tick() only performs
    // non-blocking I/O and fast crypto, so it cannot block indefinitely.
    int waited_ms = 0;
    while (handle_.load() != nullptr && waited_ms < kWorkerExitWaitMs) {
        tasks_.delayMs(10);
        waited_ms += 10;
    }

    if (handle_.load() != nullptr) {
        // Never force-delete here: the worker might hold mutex_ and
        // vTaskDelete would leave it locked forever.
        logger_.logf(LogLevel::Error,
                     "Network worker did not exit; skipping session teardown");
        return;
    }

    // 3. Worker is gone: teardown is now race-free.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (session_) {
            session_->stop();
            session_.reset();
        }
        transport_.reset();
    }

    logger_.logf(LogLevel::Info, "Network connection closed");
}

int NetworkService::send(const Bytes& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    return session_ ? session_->send(data) : -1;
}

std::optional<Bytes> NetworkService::receive() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!session_) {
        return std::nullopt;
    }
    return session_->poll();
}

bool NetworkService::isEstablished() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return session_ && session_->isEstablished();
}

uint32_t NetworkService::epoch() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return session_ ? session_->epoch() : 0;
}

} // namespace core::app
