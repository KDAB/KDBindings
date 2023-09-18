#pragma once

#include <functional>
#include <list>
#include <mutex>

namespace KDBindings {

/**
 * @brief Manages and evaluates conditions for deferred connections.
 *
 * The ConnectionEvaluator class is responsible for managing and evaluating conditions
 * that determine when deferred connections should be executed. It provides mechanisms
 * to evaluate connections based on specific criteria. This class is
 * used in conjunction with the Signal class to establish and control connections.
 */
class ConnectionEvaluator
{

public:
    ConnectionEvaluator() = default;

    ConnectionEvaluator(const ConnectionEvaluator &) noexcept = default;

    ConnectionEvaluator &operator=(const ConnectionEvaluator &) noexcept = default;

    ConnectionEvaluator(ConnectionEvaluator &&other) noexcept = delete;

    ConnectionEvaluator &operator=(ConnectionEvaluator &&other) noexcept = delete;

    /**
     * @brief Evaluate and execute deferred connections.
     *
     * This function is responsible for evaluating and executing deferred connections.
     * It locks the `connectionsMutex` to ensure thread safety while accessing the list
     * of deferred connections. It then copies the list of connections to avoid
     * interference with reentrant emissions. Finally, it iterates through the copied
     * connections and executes each one by calling the associated function object.
     */
    void evaluateDeferredConnections()
    {
        std::list<std::function<void()>> copiedConnections;
        {
            std::lock_guard<std::mutex> lock(connectionsMutex);
            copiedConnections = connections;
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