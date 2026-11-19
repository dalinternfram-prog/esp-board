#pragma once

// Session-owning network service: transport + SessionManager + worker task.
// A plain class owned by the composition root; application code sees only
// core::INetwork, so there is no global "current network" state.

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

#include "interface.hpp"

namespace core { class SessionManager; }

namespace core::app {

class NetworkService final : public INetwork {
public:
    NetworkService(ITransportFactory& factory, ILink& link, IClock& clock,
                   IRandom& random, ICrypto& crypto, ITaskRunner& tasks,
                   ILogger& logger);
    ~NetworkService() override;

    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;

    // Creates the transport/session and starts the worker task.
    void start(const char* host, const char* port) override;

    // Stops the worker and tears the session down. Never force-deletes the
    // task: killing a task that may hold a lock would leave it locked forever.
    // Must be called while the task runner is still alive.
    void close();

    // core::INetwork
    int send(const Bytes& data) override;
    std::optional<Bytes> receive() override;
    bool isEstablished() const override;
    uint32_t epoch() const override;

private:
    static void workerEntry(void* arg);
    void workerLoop();

    ITransportFactory& factory_;
    ILink&             link_;
    IClock&            clock_;
    IRandom&           random_;
    ICrypto&           crypto_;
    ITaskRunner&       tasks_;
    ILogger&           logger_;

    mutable std::mutex mutex_;
    std::unique_ptr<ITransport>     transport_;
    std::unique_ptr<SessionManager> session_;

    std::atomic<ITaskRunner::TaskId> handle_{nullptr};
    std::atomic<bool>                running_{false};
};

} // namespace core::app
