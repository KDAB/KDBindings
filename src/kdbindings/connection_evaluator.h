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
        std::vector<std::pair<const ConnectionHandle *, std::function<void()>>> movedConnections;

        {
            std::lock_guard<std::mutex> lock(connectionsMutex);

            // Move out the current connections and replace the original vector with an empty one.
            movedConnections = std::move(m_deferredConnections);
            m_deferredConnections.clear(); // This is actually redundant after a move, but makes the intent clear.
        }

        for (auto &pair : movedConnections) {
            pair.second();
        }
    }

private:
    template<typename...>
    friend class Signal;

    void enqueueSlotInvocation(const ConnectionHandle &handle, const std::function<void()> &connection)
    {
        m_deferredConnections.push_back(std::make_pair(&handle, connection));
    }

    void dequeueSlotInvocation(const ConnectionHandle &handle)
    {
        m_deferredConnections.erase(
                std::remove_if(m_deferredConnections.begin(), m_deferredConnections.end(),
                               [&handle](const auto &pair) {
                                   return pair.first == &handle;
                               }),
                m_deferredConnections.end());
    }

    std::vector<std::pair<const ConnectionHandle *, std::function<void()>>> m_deferredConnections;
    std::mutex connectionsMutex;
};
} // namespace KDBindings
