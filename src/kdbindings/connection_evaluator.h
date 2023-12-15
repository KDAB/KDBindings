#pragma once

#include <functional>
#include <list>
#include <mutex>

#include <kdbindings/connection_handle.h>

namespace KDBindings {

/**
 * @brief Manages and evaluates deferred Signal connections.
 *
 * The ConnectionEvaluator class is responsible for managing and evaluating connections
 * to Signals. It provides mechanisms to delay and control the evaluation of connections.
 * It therefore allows controlling when and on which thread slots connected to a Signal are executed.
 *
 * @see Signal::connectDeferred()
 */
class ConnectionEvaluator
{

public:
    ConnectionEvaluator() = default;

    // ConnectionEvaluator is not copyable, as it is designed to manage connections,
    // and copying it could lead to unexpected behavior, including duplication of connections and issues
    // related to connection lifetimes. Therefore, it is intentionally made non-copyable.
    ConnectionEvaluator(const ConnectionEvaluator &) noexcept = delete;

    ConnectionEvaluator &operator=(const ConnectionEvaluator &) noexcept = delete;

    // ConnectionEvaluators are not moveable, as they are captures by-reference
    // by the Signal, so moving them would lead to a dangling reference.
    ConnectionEvaluator(ConnectionEvaluator &&other) noexcept = delete;

    ConnectionEvaluator &operator=(ConnectionEvaluator &&other) noexcept = delete;

    /**
     * @brief Evaluate the deferred connections.
     *
     * This function is responsible for evaluating and executing deferred connections.
     * And this function ensures thread safety
     */
    void evaluateDeferredConnections()
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);

        for (auto &pair : m_deferredConnections) {
            pair.second();
        }
        m_deferredConnections.clear();
    }

private:
    template<typename...>
    friend class Signal;

    void enqueueSlotInvocation(const ConnectionHandle &handle, const std::function<void()> &connection)
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_deferredConnections.push_back({ handle, std::move(connection) });
    }

    void dequeueSlotInvocation(const ConnectionHandle &handle)
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);

        auto handleMatches = [&handle](const auto &invocationPair) {
            return invocationPair.first == handle;
        };

        // Remove all invocations that match the handle
        m_deferredConnections.erase(
                std::remove_if(m_deferredConnections.begin(), m_deferredConnections.end(), handleMatches),
                m_deferredConnections.end());
    }
    void disconnectAllDeferred()
    {
        // Clear the vector of deferred connections
        m_deferredConnections.clear();
    }

    std::vector<std::pair<ConnectionHandle, std::function<void()>>> m_deferredConnections;
    std::mutex m_connectionsMutex;
};
} // namespace KDBindings
