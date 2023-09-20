#pragma once

#include <functional>
#include <list>
#include <mutex>

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

    // ConnectionEvaluators are not copyable, as it is designed to manage connections,
    // and copying it could lead to unexpected behavior, including duplication of connections and issues
    // related to connection lifetimes. Therefore, it is intentionally made non-copyable.
    ConnectionEvaluator(const ConnectionEvaluator &) noexcept = delete;

    ConnectionEvaluator &operator=(const ConnectionEvaluator &) noexcept = delete;

    // ConnectionEvaluators are not moveable, as they are captures by-reference
    // by the Signal, so moving them would lead to a dangling reference.
    ConnectionEvaluator(ConnectionEvaluator &&other) noexcept = delete;

    ConnectionEvaluator &operator=(ConnectionEvaluator &&other) noexcept = delete;

    /**
     * @brief Evaluate and execute deferred connections.
     *
     * This function is responsible for evaluating and executing deferred connections.
     * And this function ensures thread safety
     */
    void evaluateDeferredConnections()
    {
        std::list<std::function<void()>> copiedConnections;
        {
            std::lock_guard<std::mutex> lock(connectionsMutex);
            copiedConnections = std::move(connections);
            // Reinitialize the connections list
            connections = std::list<std::function<void()>>();
        }

        for (auto &connection : copiedConnections) {
            connection();
        }
    }

private:
    template<typename...>
    friend class Signal;

    void addConnection(std::function<void()> connection)
    {
        connections.push_back(connection);
    }

    std::list<std::function<void()>> connections;
    std::mutex connectionsMutex;
};
} // namespace KDBindings