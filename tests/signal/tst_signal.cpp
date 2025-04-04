/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sean Harmer <sean.harmer@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "kdbindings/utils.h"
#include <kdbindings/signal.h>
#include <kdbindings/connection_evaluator.h>

#include <stdexcept>
#include <string>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

// The expansion of TEST_CASE from doctest leads to a clazy warning.
// As this issue originates from doctest, disable the warning.
// clazy:excludeall=non-pod-global-static

using namespace KDBindings;

static_assert(std::is_nothrow_destructible<Signal<int>>{});
static_assert(std::is_nothrow_default_constructible<Signal<int>>{});
static_assert(!std::is_copy_constructible<Signal<int>>{});
static_assert(!std::is_copy_assignable<Signal<int>>{});
static_assert(std::is_nothrow_move_constructible<Signal<int>>{});
static_assert(std::is_nothrow_move_assignable<Signal<int>>{});

static_assert(std::is_nothrow_destructible<ConnectionHandle>{});
static_assert(std::is_nothrow_default_constructible<ConnectionHandle>{});
static_assert(std::is_copy_constructible<ConnectionHandle>{});
static_assert(std::is_copy_assignable<ConnectionHandle>{});
static_assert(std::is_nothrow_move_constructible<ConnectionHandle>{});
static_assert(std::is_nothrow_move_assignable<ConnectionHandle>{});

static_assert(std::is_nothrow_destructible<ScopedConnection>{});
static_assert(std::is_nothrow_default_constructible<ScopedConnection>{});
static_assert(!std::is_copy_constructible<ScopedConnection>{});
static_assert(!std::is_copy_assignable<ScopedConnection>{});
static_assert(std::is_nothrow_move_constructible<ScopedConnection>{});
static_assert(std::is_nothrow_move_assignable<ScopedConnection>{});

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
        (void)s.connect(&CallbackCounter::callback, this);
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
        (void)signal.connect([&lambdaCalled](bool value) { lambdaCalled = value; });
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

        (void)signal.connect([&signalValue, &boundValue](int bound, int signalled) {
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
        (void)signal.connect(push_back, &numbers);

        // this slot doesn't require the int argument, so it will be discarded.
        (void)signal.connect([&emitted]() { emitted = true; });

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
        (void)signal.connect([&]() {
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
        (void)signal.connect([&]() {
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
        (void)signal.connect([&]() {
            ++lambdaCallCount;
        });

        int lambdaCallCount2 = 0;
        (void)signal.connect([&]() {
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

    SUBCASE("Single Shot Connection")
    {
        Signal<int> mySignal;
        int val = 5;

        // Connect a single shot slot to the signal
        auto handle = mySignal.connectSingleShot([&val](int value) {
            val += value;
        });

        mySignal.emit(5); // This should trigger the slot and then disconnect it

        REQUIRE(!handle.isActive());

        mySignal.emit(5); // Since the slot is disconnected, this should not affect 'val'

        // Check if the value remains unchanged after the second emit
        REQUIRE(val == 10); // 'val' was incremented once to 10 by the first emit and should remain at 10
    }

    SUBCASE("Self-blocking connection")
    {
        Signal<int> mySignal;
        int val = 5;

        auto handle = mySignal.connectReflective([&val](ConnectionHandle &self, int value) {
            val += value;
            self.block(true);
        });

        REQUIRE_FALSE(handle.isBlocked());
        mySignal.emit(5);
        REQUIRE(val == 10);
        REQUIRE(handle.isBlocked());

        mySignal.emit(5);
        REQUIRE(val == 10);

        handle.block(false);
        mySignal.emit(5);
        REQUIRE(val == 15);
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

    SUBCASE("A reflective connection cannot deconstruct itself while still in use")
    {
        class DestructorNotifier
        {
        public:
            DestructorNotifier(bool *flag)
                : destructorFlag(flag) { }

            ~DestructorNotifier()
            {
                if (destructorFlag) {
                    *destructorFlag = true;
                }
            }

            bool *destructorFlag = nullptr;
        };

        auto destructed = std::make_shared<bool>(false);
        auto notifier = std::make_shared<DestructorNotifier>(&*destructed);

        Signal<> signal;

        auto handle = signal.connectReflective([destructed, notifier = std::move(notifier)](ConnectionHandle &handle) {
            REQUIRE_FALSE(*destructed);
            REQUIRE(handle.isActive());
            handle.disconnect();
            // Make sure our own lambda isn't deconstructed while it's runnning.
            REQUIRE_FALSE(*destructed);
            // However, the handle will no longer be valid.
            REQUIRE_FALSE(handle.isActive());
        });

        // Make sure the notifier has actually been moved into the lambda.
        // This ensures the lambda has exclusive ownership of the DestructorNotifier.
        REQUIRE_FALSE(notifier);

        REQUIRE_FALSE(*destructed);
        REQUIRE(handle.isActive());
        signal.emit();

        REQUIRE(*destructed);
        REQUIRE_FALSE(handle.isActive());
    }
}

TEST_CASE("ConnectionEvaluator")
{
    SUBCASE("Disconnect Deferred Connection")
    {
        Signal<int> signal1;
        Signal<int, int> signal2;
        int val = 4;
        auto evaluator = std::make_shared<ConnectionEvaluator>();

        auto connection1 = signal1.connectDeferred(evaluator, [&val](int value) {
            val += value;
        });

        auto connection2 = signal2.connectDeferred(evaluator, [&val](int value1, int value2) {
            val += value1;
            val += value2;
        });

        REQUIRE(connection1.isActive());

        signal1.emit(4);
        REQUIRE(val == 4); // val not changing immediately after emit

        signal2.emit(3, 2);
        REQUIRE(val == 4); // val not changing immediately after emit

        connection1.disconnect();
        REQUIRE(!connection1.isActive());

        REQUIRE(connection2.isActive());

        evaluator->evaluateDeferredConnections(); // It doesn't evaluate any slots of signal1 as it ConnectionHandle gets disconnected before the evaluation of the deferred connections.
        REQUIRE(val == 9);
    }

    SUBCASE("Multiple Signals with Evaluator")
    {
        Signal<int> signal1;
        Signal<int> signal2;
        int val = 4;
        auto evaluator = std::make_shared<ConnectionEvaluator>();

        std::thread thread1([&] {
            (void)signal1.connectDeferred(evaluator, [&val](int value) {
                val += value;
            });
        });

        std::thread thread2([&] {
            (void)signal2.connectDeferred(evaluator, [&val](int value) {
                val += value;
            });
        });

        thread1.join();
        thread2.join();

        signal1.emit(2);
        signal2.emit(3);
        REQUIRE(val == 4); // val not changing immediately after emit

        evaluator->evaluateDeferredConnections();

        REQUIRE(val == 9);
    }

    SUBCASE("Emit Multiple Signals with Evaluator")
    {
        Signal<int> signal1;
        Signal<int> signal2;
        int val1 = 4;
        int val2 = 4;
        auto evaluator = std::make_shared<ConnectionEvaluator>();

        (void)signal1.connectDeferred(evaluator, [&val1](int value) {
            val1 += value;
        });

        (void)signal2.connectDeferred(evaluator, [&val2](int value) {
            val2 += value;
        });

        std::thread thread1([&] {
            signal1.emit(2);
        });

        std::thread thread2([&] {
            signal2.emit(3);
        });

        thread1.join();
        thread2.join();

        REQUIRE(val1 == 4);
        REQUIRE(val2 == 4);

        evaluator->evaluateDeferredConnections();

        REQUIRE(val1 == 6);
        REQUIRE(val2 == 7);
    }

    SUBCASE("Deferred Connect, Emit, Disconnect, and Evaluate")
    {
        Signal<int> signal;
        int val = 4;
        auto evaluator = std::make_shared<ConnectionEvaluator>();

        auto connection = signal.connectDeferred(evaluator, [&val](int value) {
            val += value;
        });

        REQUIRE(connection.isActive());

        signal.emit(2);
        REQUIRE(val == 4);

        connection.disconnect();
        evaluator->evaluateDeferredConnections(); // It doesn't evaluate the slot as the signal gets disconnected before it's evaluation of the deferred connections.

        REQUIRE(val == 4);
    }

    SUBCASE("Double Evaluate Deferred Connections")
    {
        Signal<int> signal;
        int val = 4;
        auto evaluator = std::make_shared<ConnectionEvaluator>();

        (void)signal.connectDeferred(evaluator, [&val](int value) {
            val += value;
        });

        signal.emit(2);
        REQUIRE(val == 4);

        evaluator->evaluateDeferredConnections();
        evaluator->evaluateDeferredConnections();

        REQUIRE(val == 6);

        signal.emit(2);
        REQUIRE(val == 6);

        evaluator->evaluateDeferredConnections();
        evaluator->evaluateDeferredConnections();

        REQUIRE(val == 8);
    }

    SUBCASE("Disconnect deferred connection from deleted signal")
    {
        auto signal = new Signal<>();
        auto anotherSignal = Signal<>();
        auto evaluator = std::make_shared<ConnectionEvaluator>();
        bool called = false;
        bool anotherCalled = false;

        auto handle = signal->connectDeferred(evaluator, [&called]() { called = true; });
        (void)anotherSignal.connectDeferred(evaluator, [&anotherCalled]() { anotherCalled = true; });

        signal->emit();
        anotherSignal.emit();
        delete signal;

        REQUIRE(!called);
        REQUIRE(!anotherCalled);
        evaluator->evaluateDeferredConnections();
        REQUIRE(!called);
        // Make sure the other signal still works, even after deconstructing another
        // signal that was connected to the same evaluator.
        REQUIRE(anotherCalled);
    }

    SUBCASE("Subclassing ConnectionEvaluator")
    {
        class MyConnectionEvaluator : public ConnectionEvaluator
        {
        protected:
            void onInvocationAdded() override
            {
                m_count++;
            }

        public:
            int m_count = 0;
        };

        Signal<> signal;

        auto evaluator = std::make_shared<MyConnectionEvaluator>();

        auto evaluated = false;
        (void)signal.connectDeferred(evaluator, [&evaluated]() { evaluated = true; });

        REQUIRE(evaluator->m_count == 0);
        signal.emit();

        REQUIRE(evaluator->m_count == 1);
        REQUIRE_FALSE(evaluated);

        evaluator->evaluateDeferredConnections();

        REQUIRE(evaluator->m_count == 1);
        REQUIRE(evaluated);
    }
}

TEST_CASE("Moving")
{
    SUBCASE("a move constructed signal keeps the connections")
    {
        int count = 0;
        auto handler = [&count]() { ++count; };

        Signal<> signal;
        (void)signal.connect(handler);

        Signal<> movedSignal{ std::move(signal) };
        movedSignal.emit();
        REQUIRE(count == 1);
    }

    SUBCASE("a move assigned signal keeps the connections")
    {
        int count = 0;
        auto handler = [&count]() { ++count; };

        Signal<> signal;
        (void)signal.connect(handler);

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

    SUBCASE("A ConnectinHandle can be released to ignore the nodiscard warning")
    {
        bool called = false;
        {
            Signal<int> mySignal;
            // As long as this compiles without error, the test can pass, no need to REQUIRE anything
            mySignal.connect([&called]() { called = true; }).release();
        }
    }
}

TEST_CASE("ScopedConnection")
{
    SUBCASE("A default constructed ScopedConnection is inactive")
    {
        ScopedConnection guard;

        REQUIRE_FALSE(guard->isActive());
    }

    SUBCASE("A ScopedConnection disconnects when it goes out of scope")
    {
        bool called = false;
        Signal<> signal;
        {
            const ScopedConnection guard = signal.connect([&called]() { called = !called; });
            REQUIRE(guard->isActive());
            signal.emit();
            REQUIRE(called);
        }
        signal.emit();
        REQUIRE(called);
    }

    SUBCASE("A ScopedConnection disconnects when assigned a new ConnectionHandle")
    {
        int numCalled = 0;
        Signal<> signal;
        ScopedConnection guard = signal.connect([&numCalled]() { numCalled++; });
        signal.emit();
        CHECK_EQ(numCalled, 1);

        guard = signal.connect([&numCalled]() { numCalled++; });
        signal.emit();
        CHECK_EQ(numCalled, 2);
    }

    SUBCASE("A ScopedConnection disconnects when move assigned")
    {
        int numCalled = 0;
        Signal<> signal;
        {
            ScopedConnection guard1 = signal.connect([&numCalled]() { numCalled++; });
            ScopedConnection guard2 = signal.connect([&numCalled]() { numCalled++; });

            // This should drop the old connection of guard1
            guard1 = std::move(guard2);
            // Ideally we'd like to assert here that:
            // CHECK(!guard2->isActive());
            // However, this is not possible, as a moved-from ScopedConnection is
            // undefined (as is any C++ object for that matter).
            // But because we assert that `guard1` is still active after the scope
            // ends and that numCalled is only 1 after the emit, we can be sure that
            // `guard2` was moved from and didn't disconnect.

            CHECK(guard1->isActive());

            signal.emit();
            CHECK_EQ(numCalled, 1);
        }
        // all connections should now be dropped
        signal.emit();
        CHECK_EQ(numCalled, 1);
    }

    SUBCASE("A ScopedConnection can be moved")
    {
        bool called = false;
        Signal<> signal;
        ScopedConnection into;
        REQUIRE_FALSE(into->isActive());
        {
            ScopedConnection guard = signal.connect([&called]() { called = true; });
            REQUIRE(guard->isActive());
            into = std::move(guard);
            // Ideally we'd like to assert here that:
            // REQUIRE_FALSE(guard->isActive());
            // However, this is not possible, as a moved-from ScopedConnection is
            // undefined (as is any C++ object for that matter).
            // But because we assert that `into` is still active after the scope
            // ends, we can be sure that `guard` was moved from and didn't disconnect.
        }
        REQUIRE(into->isActive());

        signal.emit();
        REQUIRE(called);
    }
}
