/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021-2022 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sean Harmer <sean.harmer@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "kdbindings/utils.h"
#include <kdbindings/signal.h>

#include <stdexcept>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

using namespace KDBindings;

static_assert(std::is_nothrow_destructible<Signal<int>>{});
static_assert(std::is_nothrow_default_constructible<Signal<int>>{});
static_assert(!std::is_copy_constructible<Signal<int>>{});
static_assert(!std::is_copy_assignable<Signal<int>>{});
static_assert(std::is_nothrow_move_constructible<Signal<int>>{});
static_assert(std::is_nothrow_move_assignable<Signal<int>>{});

class Button
{
public:
    Signal<> clicked;
};

class Handler
{
public:
    void doSomething()
    {
        handlerCalled = true;
    }

    bool handlerCalled = false;
};

class CallbackCounter
{
public:
    template<typename Signal>
    CallbackCounter(Signal &s)
    {
        s.connect(&CallbackCounter::callback, this);
    }

    void callback()
    {
        ++m_count;
    }

    uint32_t m_count{ 0 };
};

TEST_CASE("Signal connections")
{
    SUBCASE("A signal with arguments can be connected to a lambda and invoked")
    {
        Signal<std::string, int> signal;
        bool lambdaCalled = false;
        const auto result = signal.connect([&lambdaCalled](std::string, int) {
            lambdaCalled = true;
        });

        REQUIRE(result.isActive());

        signal.emit("The answer:", 42);
        REQUIRE(lambdaCalled == true);
    }

    SUBCASE("A signal with arguments can be connected to a lambda and invoked with l-value args")
    {
        Signal<std::string, int> signal;
        bool lambdaCalled = false;
        const auto result = signal.connect([&lambdaCalled](std::string, int) {
            lambdaCalled = true;
        });

        REQUIRE(result.isActive());

        std::string a = "The answer:";
        int b = 42;
        signal.emit(a, b);
        REQUIRE(lambdaCalled == true);
    }

    SUBCASE("A signal with arguments can be connected to a lambda and invoked with const l-value args")
    {
        Signal<std::string, int> signal;
        bool lambdaCalled = false;
        const auto result = signal.connect([&lambdaCalled](std::string, int) {
            lambdaCalled = true;
        });

        REQUIRE(result.isActive());

        const std::string a = "The answer:";
        const int b = 42;
        signal.emit(a, b);
        REQUIRE(lambdaCalled == true);
    }

    SUBCASE("A signal can be connected to a member function and invoked")
    {
        Button button;
        Handler handler;

        const auto connection = button.clicked.connect(&Handler::doSomething, &handler);
        REQUIRE(connection.isActive());

        button.clicked.emit();
        REQUIRE(handler.handlerCalled == true);
    }

    SUBCASE("A signal can discard arguments that slots don't need")
    {
        Signal<bool, int> signal;

        auto lambdaCalled = false;
        signal.connect([&lambdaCalled](bool value) { lambdaCalled = value; });
        signal.emit(true, 5);
        REQUIRE(lambdaCalled);

        signal.emit(false, 5);
        REQUIRE_FALSE(lambdaCalled);
    }

    SUBCASE("A signal can bind arbitrary arguments to the first arguments of a slot")
    {
        Signal<int, bool> signal;
        auto signalValue = 0;
        auto boundValue = 0;

        signal.connect([&signalValue, &boundValue](int bound, int signalled) {
            boundValue = bound;
            signalValue = signalled;
        },
                       5);

        // The bound value should not have changed yet.
        REQUIRE(boundValue == 0);

        signal.emit(10, false);

        REQUIRE(boundValue == 5);
        REQUIRE(signalValue == 10);
    }

    SUBCASE("Test Signal documentation example for Signal::connect<>")
    {
        Signal<int> signal;
        std::vector<int> numbers{ 1, 2, 3 };
        bool emitted = false;

        // disambiguation necessary, as push_back is overloaded.
        void (std::vector<int>::*push_back)(const int &) = &std::vector<int>::push_back;
        signal.connect(push_back, &numbers);

        // this slot doesn't require the int argument, so it will be discarded.
        signal.connect([&emitted]() { emitted = true; });

        signal.emit(4); // Will add 4 to the vector and set emitted to true

        REQUIRE(emitted);
        REQUIRE(numbers.back() == 4);
        REQUIRE(numbers.size() == 4);
    }

    SUBCASE("A signal can be disconnected after being connected")
    {
        Signal<> signal;
        int lambdaCallCount = 0;
        auto result = signal.connect([&]() {
            ++lambdaCallCount;
        });

        int lambdaCallCount2 = 0;
        signal.connect([&]() {
            ++lambdaCallCount2;
        });

        signal.emit();
        REQUIRE(lambdaCallCount == 1);
        REQUIRE(lambdaCallCount2 == 1);

        result.disconnect();

        signal.emit();
        REQUIRE(lambdaCallCount == 1);
        REQUIRE(lambdaCallCount2 == 2);
    }

    SUBCASE("A signal can be disconnected inside a slot")
    {
        Signal<> signal;
        ConnectionHandle *handle = nullptr;

        int lambdaCallCount = 0;
        auto result = signal.connect([&]() {
            ++lambdaCallCount;
            handle->disconnect();
        });
        handle = &result;

        int lambdaCallCount2 = 0;
        signal.connect([&]() {
            ++lambdaCallCount2;
        });

        signal.emit();
        REQUIRE(lambdaCallCount == 1);
        REQUIRE(lambdaCallCount2 == 1);

        signal.emit();
        REQUIRE(lambdaCallCount == 1);
        REQUIRE(lambdaCallCount2 == 2);
    }

    SUBCASE("All signal slots can be disconnected simultaneously")
    {
        Signal<> signal;
        int lambdaCallCount = 0;
        signal.connect([&]() {
            ++lambdaCallCount;
        });

        int lambdaCallCount2 = 0;
        signal.connect([&]() {
            ++lambdaCallCount2;
        });

        signal.emit();
        REQUIRE(lambdaCallCount == 1);
        REQUIRE(lambdaCallCount2 == 1);

        signal.disconnectAll();

        signal.emit();
        REQUIRE(lambdaCallCount == 1);
        REQUIRE(lambdaCallCount2 == 1);
    }

    SUBCASE("A signal can be connected via a non-const reference to it")
    {
        Signal<int> s;
        CallbackCounter counter(s);

        s.emit(1);
        s.emit(2);
        s.emit(3);

        REQUIRE(counter.m_count == 3);
    }
}

TEST_CASE("Moving")
{
    SUBCASE("a move constructed signal keeps the connections")
    {
        int count = 0;
        auto handler = [&count]() { ++count; };

        Signal<> signal;
        signal.connect(handler);

        Signal<> movedSignal{ std::move(signal) };
        movedSignal.emit();
        REQUIRE(count == 1);
    }

    SUBCASE("a move assigned signal keeps the connections")
    {
        int count = 0;
        auto handler = [&count]() { ++count; };

        Signal<> signal;
        signal.connect(handler);

        Signal<> movedSignal = std::move(signal);
        movedSignal.emit();
        REQUIRE(count == 1);
    }

    SUBCASE("A move assigned signal preserves its connection handles")
    {
        Signal<> signal;
        const auto handle = signal.connect([]() {});

        // use unique_ptr to make sure the location of the signal changes
        auto movedSignal = std::make_unique<Signal<>>(std::move(signal));
        REQUIRE(movedSignal->isConnectionBlocked(handle) == false);
    }
}

TEST_CASE("Connection blocking")
{
    SUBCASE("can block a connection")
    {
        int count = 0;
        auto handler = [&count]() { ++count; };
        Signal<> signal;
        auto connectionHandle = signal.connect(handler);
        REQUIRE(signal.isConnectionBlocked(connectionHandle) == false);

        const auto wasBlocked = signal.blockConnection(connectionHandle, true);
        REQUIRE(signal.isConnectionBlocked(connectionHandle) == true);

        signal.emit();
        REQUIRE(count == 0);

        const auto wasBlocked2 = signal.blockConnection(connectionHandle, wasBlocked);
        REQUIRE(wasBlocked2 == true);
        REQUIRE(signal.isConnectionBlocked(connectionHandle) == false);
    }

    SUBCASE("unblocking a deleted connection throws an exception")
    {
        auto handler = []() {};
        Signal<> signal;
        const auto handle = signal.connect(handler);

        signal.disconnect(handle);
        REQUIRE_THROWS_AS(signal.blockConnection(handle, true), std::out_of_range);

        REQUIRE_THROWS_AS(signal.isConnectionBlocked(handle), std::out_of_range);
    }

    SUBCASE("Creating a ConnectionBlocker for a deleted connection throws an exception")
    {
        auto handler = []() {};
        Signal<> signal;
        const auto handle = signal.connect(handler);

        signal.disconnect(handle);

        REQUIRE_THROWS_AS(ConnectionBlocker blocker(handle), std::out_of_range);
    }

    SUBCASE("can block a connection with a ConnectionBlocker")
    {
        int count = 0;
        auto handler = [&count]() { ++count; };
        Signal<> signal;
        const auto handle = signal.connect(handler);

        {
            ConnectionBlocker blocker(handle);
            REQUIRE(signal.isConnectionBlocked(handle) == true);
            signal.emit();
            REQUIRE(count == 0);
        }

        REQUIRE(signal.isConnectionBlocked(handle) == false);
    }

    SUBCASE("ConnectionBlocker leaves already blocked connections blocked")
    {
        int count = 0;
        auto handler = [&count]() { ++count; };
        Signal<> signal;
        const auto handle = signal.connect(handler);

        signal.blockConnection(handle, true);
        REQUIRE(signal.isConnectionBlocked(handle) == true);

        {
            ConnectionBlocker blocker(handle);
            REQUIRE(signal.isConnectionBlocked(handle) == true);
        }

        REQUIRE(signal.isConnectionBlocked(handle) == true);
    }
}

TEST_CASE("ConnectionHandle")
{
    SUBCASE("A default constructed ConnectionHandle is not active")
    {
        ConnectionHandle handle;
        REQUIRE_FALSE(handle.isActive());
    }

    // Regression test, initial implementation of belongsTo returned true
    // if an empty ConnectionHandle was tested with an empty Signal
    SUBCASE("Default constructed ConnectionHandle doesn't belong to any Signal")
    {
        ConnectionHandle handle;
        Signal emptySignal;
        REQUIRE_FALSE(handle.belongsTo(emptySignal));
    }

    SUBCASE("can disconnect a slot")
    {
        auto called = false;
        Signal<> signal;
        auto handle = signal.connect([&called]() { called = true; });

        handle.disconnect();
        signal.emit();

        REQUIRE_FALSE(called);
    }

    SUBCASE("becomes inactive after disconnect")
    {
        Signal<> signal;
        auto handle = signal.connect([]() {});
        auto handleCopy = handle;

        REQUIRE(handle.isActive());
        REQUIRE(handleCopy.isActive());
        handle.disconnect();
        REQUIRE_FALSE(handle.isActive());
        REQUIRE_FALSE(handleCopy.isActive());

        handle = signal.connect([]() {});

        REQUIRE(handle.isActive());
        signal.disconnect(handle);
        REQUIRE_FALSE(handle.isActive());
    }

    SUBCASE("can (un-)block its connection")
    {
        Signal<> signal;
        auto handle = signal.connect([]() {});

        REQUIRE_FALSE(handle.block(true));
        REQUIRE(handle.isBlocked());
        REQUIRE(signal.isConnectionBlocked(handle));

        REQUIRE(handle.block(false));
        REQUIRE_FALSE(handle.isBlocked());
        REQUIRE_FALSE(signal.isConnectionBlocked(handle));
    }

    SUBCASE("becomes inactive if the Signal is deleted")
    {
        auto signal = new Signal<>();
        auto handle = signal->connect([]() {});

        REQUIRE(handle.isActive());
        delete signal;
        REQUIRE_FALSE(handle.isActive());
    }

    SUBCASE("can double disconnect without problem")
    {
        Signal<> signal;
        auto handle = signal.connect([]() {});

        REQUIRE(handle.isActive());
        handle.disconnect();
        REQUIRE_FALSE(handle.isActive());

        REQUIRE_NOTHROW(handle.disconnect());
        REQUIRE_FALSE(handle.isActive());
    }

    SUBCASE("knows the signal it belongs to")
    {
        Signal signal;
        Signal otherSignal;

        auto handle = signal.connect([]() {});
        REQUIRE(handle.belongsTo(signal));
        REQUIRE_FALSE(handle.belongsTo(otherSignal));

        otherSignal = std::move(signal);
        REQUIRE_FALSE(handle.belongsTo(signal));
        REQUIRE(handle.belongsTo(otherSignal));
    }

    SUBCASE("scoped connection handle")
    {
        bool called = false;
        Signal<> signal;
        {
            ScopedConnectionHandle handle = signal.connect([&called]() { called = !called; });
            REQUIRE(handle.handle().isActive());
            signal.emit();
            REQUIRE(called);
        }
        signal.emit();
        REQUIRE(called);
    }
}
