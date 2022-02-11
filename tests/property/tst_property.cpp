/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021-2022 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sean Harmer <sean.harmer@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include <kdbindings/property.h>

#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

using namespace KDBindings;

static_assert(std::is_nothrow_destructible<Property<int>>{});
static_assert(std::is_default_constructible<Property<int>>{});
static_assert(!std::is_copy_constructible<Property<int>>{});
static_assert(!std::is_copy_assignable<Property<int>>{});
// A property cannot be nothrow_move_assignable/constructible, as it emits
// a moved Signal when moved. Any slot that is connected to this Signal may throw.
static_assert(!std::is_nothrow_move_constructible<Property<int>>{});
static_assert(!std::is_nothrow_move_assignable<Property<int>>{});


struct CustomType {
    CustomType(int _a, uint64_t _b)
        : a(_a), b(_b)
    {
    }

    bool operator==(const CustomType &other) const
    {
        return a == other.a && b == other.b;
    }

    int a{ 0 };
    uint64_t b{ 0 };
};

TEST_CASE("A property can be written to")
{
    SUBCASE("Builtin type")
    {
        Property<int> property(3);
        property = 7;
        REQUIRE(property.get() == 7);
    }

    SUBCASE("Custom type")
    {
        Property<CustomType> property(CustomType(3, 4));
        property = { 6, 14 };
        REQUIRE(property.get() == CustomType(6, 14));
    }
}

struct ObjectWithSignal {
    void emitSignal() const
    {
        valueChanged.emit();
    }

    int value{ 0 };
    mutable Signal<> valueChanged;
};

class ClassWithProperty
{
public:
    Property<ObjectWithSignal> property;
    Signal<> changed;

    ClassWithProperty()
    {
        property().valueChanged.connect(
                [this]() {
                    this->changed.emit();
                });
    }
};

TEST_CASE("An object with a Signal that is wrapped in a property can emit the Signal if it is mutable")
{
    bool called = false;
    auto handler = [&called]() { called = true; };

    ClassWithProperty outer;
    outer.changed.connect(handler);

    outer.property().emitSignal();

    REQUIRE(called == true);
}

class Handler
{
public:
    void doSomething(const int & /*value*/)
    {
        handlerCalled = true;
    }

    bool handlerCalled = false;
};

class HandlerAboutToChange
{
public:
    void doSomething(const int & /*oldValue*/, const int & /*newValue*/)
    {
        handlerCalled = true;
    }

    bool handlerCalled = false;
};

TEST_CASE("Signals")
{
    SUBCASE("A property does not emit a signal when the value set is equal to the current value")
    {
        Property<int> property(3);
        Handler handler;
        HandlerAboutToChange aboutToChangeHandler;

        property.valueChanged().connect(&Handler::doSomething, &handler);
        property.valueAboutToChange().connect(&HandlerAboutToChange::doSomething, &aboutToChangeHandler);

        property = 3;
        REQUIRE(property.get() == 3);
        REQUIRE_FALSE(handler.handlerCalled);
        REQUIRE_FALSE(aboutToChangeHandler.handlerCalled);
    }

    SUBCASE("A property does emit a signal when the value changes")
    {
        Property<int> property(3);
        Handler handler;
        HandlerAboutToChange aboutToChangeHandler;

        property.valueChanged().connect(&Handler::doSomething, &handler);
        property.valueAboutToChange().connect(&HandlerAboutToChange::doSomething, &aboutToChangeHandler);

        property = 7;
        REQUIRE(property.get() == 7);
        REQUIRE(handler.handlerCalled);
        REQUIRE(aboutToChangeHandler.handlerCalled);
    }

    SUBCASE("A property emits the destroyed signal when it is destroyed")
    {
        bool notified = false;
        auto handler = [&notified]() { notified = true; };

        auto p = new Property<int>{ 5 };
        p->destroyed().connect(handler);

        delete p;
        REQUIRE(notified == true);
    }
}

struct EqualityTestStruct {
    int value;
};

// This equal_to specialization makes sure only EqualityTestStructs with
// an increasing value can be assigned to the property
namespace KDBindings {
template<>
struct equal_to<EqualityTestStruct> {
    bool operator()(const EqualityTestStruct &a, const EqualityTestStruct &b)
    {
        return a.value < b.value;
    }
};
} // namespace KDBindings

TEST_CASE("Equality")
{
    SUBCASE("the equal_to function template object can be specialized to implement custom equality behavior for properties")
    {
        auto callCount = 0;

        Property<EqualityTestStruct> property(EqualityTestStruct{ 0 });

        property.valueChanged().connect([&callCount]() { ++callCount; });

        property = EqualityTestStruct{ 1 };
        REQUIRE(callCount == 1);
        REQUIRE(property.get().value == 1);

        property = EqualityTestStruct{ -1 };
        REQUIRE(callCount == 1);
        REQUIRE(property.get().value == 1);
    }
}

class DummyPropertyUpdater : public PropertyUpdater<int>
{
public:
    DummyPropertyUpdater(int value)
        : PropertyUpdater()
        , m_value{ value }
    {
    }

    void setUpdateFunction(std::function<void(int &&)> const &updateFunction) override
    {
        m_updateFunction = updateFunction;
    }

    int get() const override { return m_value; }

    void set(int value)
    {
        m_value = value;
        m_updateFunction(std::move(m_value));
    }

private:
    std::function<void(int &&)> m_updateFunction;
    int m_value;
};

TEST_CASE("Property Updators")
{
    SUBCASE("Can construct a property with an updater and the property assumes its value")
    {
        auto updater = std::make_unique<DummyPropertyUpdater>(42);
        auto property = Property<int>(std::move(updater));
        REQUIRE(property.get() == 42);
    }

    SUBCASE("A property with an updater throws when attempting to set it directly")
    {
        Property<int> property(std::make_unique<DummyPropertyUpdater>(7));
        REQUIRE_THROWS_AS((property = 4), ReadOnlyProperty);
    }

    SUBCASE("A property with an updater notifies when it is updated via the updater")
    {
        auto updater = new DummyPropertyUpdater(7);
        Property<int> property{ std::unique_ptr<DummyPropertyUpdater>(updater) };
        bool slotCalled = false;
        int updatedValue = 0;
        auto handler = [&slotCalled, &updatedValue](int value) {
            updatedValue = value;
            slotCalled = true;
        };
        property.valueChanged().connect(handler);

        updater->set(123);
        REQUIRE(property.get() == 123);
        REQUIRE(slotCalled);
        REQUIRE(updatedValue == 123);
    }
}

TEST_CASE("Moving")
{
    SUBCASE("move constructed property holds the correct value")
    {
        auto property = Property<std::unique_ptr<int>>{ std::make_unique<int>(42) };
        auto movedToProperty = Property<std::unique_ptr<int>>{ std::move(property) };

        REQUIRE(*(movedToProperty.get()) == 42);
    }

    SUBCASE("move assigned property holds the correct value")
    {
        auto property = Property<std::unique_ptr<int>>{ std::make_unique<int>(42) };
        auto movedToProperty = std::move(property);

        REQUIRE(*(movedToProperty.get()) == 42);
    }

    SUBCASE("move constructed property maintains connections")
    {
        int countVoid = 0;
        auto handlerVoid = [&countVoid]() { ++countVoid; };
        int countValue = 0;
        auto handlerValue = [&countValue](const std::unique_ptr<int> &) { ++countValue; };

        auto property = Property<std::unique_ptr<int>>{ std::make_unique<int>(42) };
        property.valueChanged().connect(handlerVoid);
        property.valueChanged().connect(handlerValue);

        auto movedProperty{ std::move(property) };
        movedProperty.set(std::make_unique<int>(123));

        REQUIRE(countVoid == 1);
        REQUIRE(countValue == 1);
        REQUIRE(*(movedProperty.get()) == 123);
    }

    SUBCASE("a property notifies when it has been moved into so we can recreate connections")
    {
        int countVoid = 0;
        auto handlerVoid = [&countVoid]() { ++countVoid; };
        Property<int> property{ 42 };

        // Create a lambda we can use to create the initial connection and restore it
        // when the property gets moved into (e.g. most likely when we assign a binding)
        int countMoved = 0;
        auto makeConnection = [&handlerVoid, &countMoved](const Property<int> &p) {
            ++countMoved;
            p.valueChanged().connect(handlerVoid);
        };

        // Create the initial connection
        makeConnection(property);
        REQUIRE(countMoved == 1);

        // Ensure we get told about any moves
        property.moved().connect(makeConnection);

        // Change the property
        property = 64;
        REQUIRE(countVoid == 1);

        // Move a new property into our original property
        Property<int> property2{ 5 };
        property = std::move(property2);
        REQUIRE(countMoved == 2);

        // Change the property again and our original lamba should get called still
        property = 128;
        REQUIRE(countVoid == 2);
    }
}
