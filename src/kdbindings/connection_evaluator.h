/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Shivam Kunwar <shivam.kunwar@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/
#pragma once

#include <algorithm>
#include <functional>
#include <mutex>

#include <kdbindings/connection_handle.h>

namespace KDBindings {

/**
 * @brief Manages and evaluates deferred Signal connections.
 *
 * @warning Deferred connections are experimental and may be removed or changed in the future.
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
    /** ConnectionEvaluators are default constructible */
    ConnectionEvaluator() = default;

    /** Connectionevaluators are not copyable */
    // As it is designed to manage connections,
    // and copying it could lead to unexpected behavior, including duplication of connections and issues
    // related to connection lifetimes. Therefore, it is intentionally made non-copyable.
    ConnectionEvaluator(const ConnectionEvaluator &) noexcept = delete;

    ConnectionEvaluator &operator=(const ConnectionEvaluator &) noexcept = delete;

    /** ConnectionEvaluators are not moveable */
    // As they are captures by-reference
    // by the Signal, so moving them would lead to a dangling reference.
    ConnectionEvaluator(ConnectionEvaluator &&other) noexcept = delete;

    ConnectionEvaluator &operator=(ConnectionEvaluator &&other) noexcept = delete;

    /**
     * @brief Evaluate the deferred connections.
     *
     * This function is responsible for evaluating and executing deferred connections.
     * This function is thread safe.
     */
    void evaluateDeferredConnections()
    {
        std::lock_guard<std::mutex> lock(m_slotInvocationMutex);

        for (auto &pair : m_deferredSlotInvocations) {
            pair.second();
        }
        m_deferredSlotInvocations.clear();
    }

private:
    template<typename...>
    friend class Signal;

    void enqueueSlotInvocation(const ConnectionHandle &handle, const std::function<void()> &slotInvocation)
    {
        std::lock_guard<std::mutex> lock(m_slotInvocationMutex);
        m_deferredSlotInvocations.push_back({ handle, std::move(slotInvocation) });
    }

    void dequeueSlotInvocation(const ConnectionHandle &handle)
    {
        std::lock_guard<std::mutex> lock(m_slotInvocationMutex);

        auto handleMatches = [&handle](const auto &invocationPair) {
            return invocationPair.first == handle;
        };

        // Remove all invocations that match the handle
        m_deferredSlotInvocations.erase(
                std::remove_if(m_deferredSlotInvocations.begin(), m_deferredSlotInvocations.end(), handleMatches),
                m_deferredSlotInvocations.end());
    }

    std::vector<std::pair<ConnectionHandle, std::function<void()>>> m_deferredSlotInvocations;
    std::mutex m_slotInvocationMutex;
};
} // namespace KDBindings
