/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sean Harmer <sean.harmer@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "kdbindings/make_node.h"
#include <kdbindings/binding.h>
#include <kdbindings/binding_evaluator.h>
#include <kdbindings/node_operators.h>
#include <kdbindings/node_functions.h>

#include <iostream>
#include <string>
#include <type_traits>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

// The expansion of TEST_CASE from doctest leads to a clazy warning.
// As this issue originates from doctest, disable the warning.
// clazy:excludeall=non-pod-global-static

using namespace KDBindings;

static_assert(std::is_destructible<Binding<int>>{});
static_assert(!std::is_default_constructible<Binding<int>>{});
static_assert(!std::is_copy_constructible<Binding<int>>{});
static_assert(!std::is_copy_assignable<Binding<int>>{});
static_assert(!std::is_nothrow_move_constructible<Binding<int>>{});
static_assert(!std::is_nothrow_move_assignable<Binding<int>>{});

static_assert(std::is_destructible<BindingEvaluator>{});
static_assert(std::is_default_constructible<BindingEvaluator>{});
static_assert(std::is_nothrow_copy_constructible<BindingEvaluator>{});
static_assert(std::is_nothrow_copy_assignable<BindingEvaluator>{});
static_assert(!std::is_move_constructible<BindingEvaluator>{});
static_assert(!std::is_move_assignable<BindingEvaluator>{});

TEST_CASE("Binding construction and basic evaluation")
{
    SUBCASE("can construct a manual mode binding")
    {
        auto evaluator = BindingEvaluator{};
        auto binding = Binding<int>{ Private::makeNode(7), evaluator };

        REQUIRE(binding.get() == 7);
    }

    SUBCASE("can construct a property from a binding")
    {
        auto evaluator = BindingEvaluator{};
        auto property = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode(42), evaluator)
        };

        REQUIRE(property.get() == 42);
    }

    SUBCASE("assigning to a property constructed from a binding throws")
    {
        auto evaluator = BindingEvaluator{};
        auto property = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode(42), evaluator)
        };

        // No more broken bindings!
        REQUIRE_THROWS_AS((property = 3), ReadOnlyProperty);
    }
}

TEST_CASE("Binding expression updates")
{
    SUBCASE("change to input property is reflected in binding and output property")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> inputProperty(5);
        auto outputProperty = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode([](int x) { return x * x; },
                                      Private::makeNode(inputProperty)),
                    evaluator)
        };

        REQUIRE(inputProperty.get() == 5);
        REQUIRE(outputProperty.get() == 25);

        // Update the input property and the binding should allow the output property
        // to get the new value when we poke the evaluator.
        inputProperty = 8;
        REQUIRE(inputProperty.get() == 8);
        REQUIRE(outputProperty.get() == 25);

        evaluator.evaluateAll();
        REQUIRE(inputProperty.get() == 8);
        REQUIRE(outputProperty.get() == 64);
    }

    SUBCASE("can update multiple bindings with independent inputs")
    {
        auto evaluator = BindingEvaluator{};

        Property<int> input1(5);
        auto prop1 = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode([](auto x) { return x * x; },
                                      Private::makeNode(input1)),
                    evaluator)
        };

        Property<int> input2(6);
        auto prop2 = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode([](auto x) { return 3 * x; },
                                      Private::makeNode(input2)),
                    evaluator)
        };

        REQUIRE(prop1.get() == 25);
        REQUIRE(prop2.get() == 18);

        input1 = 4;
        input2 = 12;

        // Output properties still have old values until we evaluate
        REQUIRE(prop1.get() == 25);
        REQUIRE(prop2.get() == 18);

        evaluator.evaluateAll();

        REQUIRE(prop1.get() == 16);
        REQUIRE(prop2.get() == 36);
    }

    SUBCASE("can update multiple bindings with common inputs")
    {
        auto evaluator = BindingEvaluator{};

        Property<int> input(5);
        auto prop1 = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode([](auto x) { return x * x; },
                                      Private::makeNode(input)),
                    evaluator)
        };

        auto prop2 = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode([](auto x) { return 3 * x; },
                                      Private::makeNode(input)),
                    evaluator)
        };

        REQUIRE(prop1.get() == 25);
        REQUIRE(prop2.get() == 15);

        input = 8;

        // Output properties still have old values until we evaluate
        REQUIRE(prop1.get() == 25);
        REQUIRE(prop2.get() == 15);

        evaluator.evaluateAll();

        REQUIRE(prop1.get() == 64);
        REQUIRE(prop2.get() == 24);
    }

    SUBCASE("Bindings are evaluated in the order they were created (within an evaluator)")
    {
        auto evaluator = BindingEvaluator{};
        std::vector<int> ordering;

        Property<int> input(5);
        auto prop1 = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode([](auto x) { return x * x; },
                                      Private::makeNode(input)),
                    evaluator)
        };
        (void)prop1.valueChanged().connect([&ordering](int) { ordering.emplace_back(1); });

        auto prop2 = Property<int>{
            std::make_unique<Binding<int>>(
                    Private::makeNode([](auto x) { return 3 * x; },
                                      Private::makeNode(input)),
                    evaluator)
        };
        (void)prop2.valueChanged().connect([&ordering](int) { ordering.emplace_back(2); });

        input = 8;
        evaluator.evaluateAll();

        REQUIRE(ordering.size() == 2);
        REQUIRE(ordering[0] == 1);
        REQUIRE(ordering[1] == 2);
    }
}

TEST_CASE("Create property bindings using helper function")
{
    SUBCASE("can construct a manual mode binding wrapping a constant")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> property = makeBoundProperty(evaluator, Private::makeNode(7));

        REQUIRE(property.get() == 7);
    }

    SUBCASE("can construct a manual mode binding wrapping a property")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> inputProperty{ 18 };
        Property<int> property = makeBoundProperty(evaluator, Private::makeNode(inputProperty));

        REQUIRE(property.get() == 18);
    }

    SUBCASE("can construct a manual mode binding wrapping a function node")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> inputProperty{ 5 };
        auto property = makeBoundProperty(
                evaluator,
                Private::makeNode([](auto x) { return x * x; },
                                  Private::makeNode(inputProperty)));

        REQUIRE(inputProperty.get() == 5);
        REQUIRE(property.get() == 25);
    }

    SUBCASE("binding created with helper function reflects new value when input changed and evaluated")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> inputProperty{ 5 };
        auto property = makeBoundProperty(
                evaluator,
                Private::makeNode([](auto x) { return x * x; },
                                  Private::makeNode(inputProperty)));

        inputProperty = 7;
        REQUIRE(property.get() == 25); // Holds old value until we trigger an evaluation
        evaluator.evaluateAll();
        REQUIRE(property.get() == 49);
    }

    SUBCASE("can construct a manual mode binding directly from another property")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> inputProperty{ 5 };
        auto property = makeBoundProperty(evaluator, inputProperty);

        REQUIRE(property.get() == 5);
    }

    SUBCASE("assigning to a property with a binding throws")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> inputProperty{ 5 };
        auto property = makeBoundProperty(evaluator, inputProperty);
        REQUIRE_THROWS_AS((property = 123), ReadOnlyProperty);
    }

    SUBCASE("can construct an immediate mode binding wrapping a constant")
    {
        Property<int> property = makeBoundProperty(Private::makeNode(7));

        REQUIRE(property.get() == 7);
    }

    SUBCASE("can construct an immediate mode binding wrapping a property")
    {
        Property<int> inputProperty{ 18 };
        Property<int> property = makeBoundProperty(Private::makeNode(inputProperty));

        REQUIRE(property.get() == 18);
    }

    SUBCASE("can construct an immediate mode binding wrapping a function node")
    {
        Property<int> inputProperty{ 5 };
        auto property = makeBoundProperty(
                Private::makeNode([](auto x) { return x * x; },
                                  Private::makeNode(inputProperty)));

        REQUIRE(inputProperty.get() == 5);
        REQUIRE(property.get() == 25);
    }

    SUBCASE("can construct a binding to a pointer")
    {
        int integer = 7;
        int anotherInt = 10;
        Property<int *> property(&integer);
        auto boundProperty = makeBoundProperty(property);

        static_assert(std::is_same_v<decltype(boundProperty), Property<int *>>);

        REQUIRE(*boundProperty.get() == 7);

        integer = 9;
        REQUIRE(*boundProperty.get() == 9);

        property.set(&anotherInt);
        REQUIRE(*boundProperty.get() == 10);
    }
}

TEST_CASE("Binding reassignment")
{
    SUBCASE("A binding can not be override by a constant value")
    {
        Property<int> source(0);

        auto bound = makeBoundProperty(source);
        REQUIRE_THROWS_AS(bound = 1, ReadOnlyProperty);
    }

    SUBCASE("A binding can be replaced by another binding")
    {
        Property<int> source(0);

        auto bound = makeBoundProperty(source);

        Property<int> anotherSource(1);
        bound = makeBinding(anotherSource);

        REQUIRE(bound.get() == 1);
    }

    SUBCASE("Reassigning a binding keeps signal connections")
    {
        bool called = false;

        Property<int> source(0);
        auto bound = makeBoundProperty(source);

        bound.valueChanged().connect([&called]() { called = true; });

        REQUIRE_FALSE(called);

        Property<int> anotherSource(1);
        bound = makeBinding(anotherSource);

        REQUIRE(called);

        called = false;

        anotherSource = 10;
        REQUIRE(called);
    }

    SUBCASE("A binding can be broken by calling `reset`")
    {
        Property<int> value(2);

        auto result = makeBoundProperty(2 * value);

        REQUIRE(result.get() == 4);

        result.reset();
        value = 4;

        REQUIRE(result.get() == 4);
    }
}

TEST_CASE("Expression node tree construction: operators")
{
    SUBCASE("Unary op -")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> p{ 8 };
        auto q = makeBoundProperty(evaluator, -p);

        REQUIRE(q.get() == -8);
    }

    SUBCASE("Unary op !")
    {
        auto evaluator = BindingEvaluator{};
        Property<bool> p{ false };
        auto q = makeBoundProperty(evaluator, !p);

        REQUIRE(q.get() == true);
    }

    SUBCASE("Unary op +")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> p{ 8 };
        auto q = makeBoundProperty(evaluator, +p);

        REQUIRE(q.get() == 8);
    }

    SUBCASE("Unary op - [immediate mode]")
    {
        Property<int> p{ 8 };
        auto q = makeBoundProperty(-p);

        REQUIRE(q.get() == -8);
    }

    SUBCASE("Unary op ! [immediate mode]")
    {
        Property<bool> p{ false };
        auto q = makeBoundProperty(!p);

        REQUIRE(q.get() == true);
    }

    SUBCASE("Unary op + [immediate mode]")
    {
        Property<int> p{ 8 };
        auto q = makeBoundProperty(+p);

        REQUIRE(q.get() == 8);
    }
}

TEST_CASE("Expression node tree construction: binary operators")
{
    SUBCASE("Binary op + (Property, Property)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ 8 };
        Property<int> b{ 7 };
        auto x = makeBoundProperty(evaluator, a + b);

        REQUIRE(x.get() == 15);
    }

    SUBCASE("Binary op + (Property, value)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ 4 };
        int b{ 12 };
        auto x = makeBoundProperty(evaluator, a + b);

        REQUIRE(x.get() == 16);
    }

    SUBCASE("Binary op + (value, Property)")
    {
        auto evaluator = BindingEvaluator{};
        int a{ 12 };
        Property<int> b{ 4 };
        auto x = makeBoundProperty(evaluator, a + b);

        REQUIRE(x.get() == 16);
    }

    SUBCASE("Binary op + (Property, Node)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ 2 };
        Property<int> b{ 7 };
        Property<int> c{ 3 };
        auto x = makeBoundProperty(evaluator, a * (b + c));

        REQUIRE(x.get() == 20);
    }

    SUBCASE("Binary op + (Node, Property)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ 2 };
        Property<int> b{ 7 };
        Property<int> c{ 3 };
        auto x = makeBoundProperty(evaluator, (b + c) * a);

        REQUIRE(x.get() == 20);
    }

    SUBCASE("Binary op + (value, Node)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> b{ 7 };
        Property<int> c{ 3 };
        auto x = makeBoundProperty(evaluator, 2 * (b + c));

        REQUIRE(x.get() == 20);
    }

    SUBCASE("Binary op + (Node, value)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> b{ 7 };
        Property<int> c{ 3 };
        auto x = makeBoundProperty(evaluator, (b + c) * 2);

        REQUIRE(x.get() == 20);
    }

    SUBCASE("Binary op + (Node, Node)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ 2 };
        Property<int> b{ 7 };
        Property<int> c{ 3 };
        Property<int> d{ 4 };
        auto x = makeBoundProperty(evaluator, (a + b) * (c + d));

        REQUIRE(x.get() == 63);
    }

    SUBCASE("Binary op + (Property, Property) [immediate mode]")
    {
        Property<int> a{ 8 };
        Property<int> b{ 7 };
        auto x = makeBoundProperty(a + b);

        REQUIRE(x.get() == 15);
    }
}

struct node_paraboloid {
    template<typename T>
    auto operator()(T &&x, T &&y) const
    {
        return x * x + y * y;
    }
};

KDBINDINGS_DECLARE_FUNCTION(paraboloid, node_paraboloid{})

struct make_title_functor {
    template<typename T>
    auto operator()(T &&w, T &&h) const
    {
        std::ostringstream ss;
        ss << "Size = " << w << " x " << h;
        return ss.str();
    }
};
KDBINDINGS_DECLARE_FUNCTION(makeTitle, make_title_functor{})

auto make_title_lambda = [](uint32_t w, uint32_t h) {
    std::ostringstream ss;
    ss << "Size = " << w << " x " << h;
    return ss.str();
};
KDBINDINGS_DECLARE_FUNCTION(makeTitleLambda, make_title_lambda)

namespace Foo {
template<typename T>
T bar(T x) noexcept
{
    return 2 * x + 1;
}

int bar(int x, int factor)
{
    return factor * x + 1;
}
} // namespace Foo
KDBINDINGS_DECLARE_NAMESPACED_FUNCTION(Foo, bar)

KDBINDINGS_DECLARE_STD_FUNCTION(max)

TEST_CASE("Expression node tree construction: functions")
{
    SUBCASE("Unary function (Property)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ -23 };
        auto x = makeBoundProperty(evaluator, abs(a));

        REQUIRE(x.get() == 23);
    }

    SUBCASE("Custom binary function")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> x{ -2 };
        Property<int> y{ 3 };
        auto z = makeBoundProperty(evaluator, paraboloid(x, y));

        REQUIRE(z.get() == 13);
    }

    SUBCASE("Custom binary function with different return type")
    {
        auto evaluator = BindingEvaluator{};
        Property<uint32_t> width{ 800 };
        Property<uint32_t> height{ 600 };
        auto z = makeBoundProperty(evaluator, makeTitle(width, height));

        REQUIRE(z.get() == "Size = 800 x 600");
    }

    SUBCASE("Custom lambda with different return type")
    {
        auto evaluator = BindingEvaluator{};
        Property<uint32_t> width{ 800 };
        Property<uint32_t> height{ 600 };
        auto z = makeBoundProperty(evaluator, makeTitleLambda(width, height));

        REQUIRE(z.get() == "Size = 800 x 600");
    }

    SUBCASE("Custom namespaced function (Property)")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ 5 };
        auto x = makeBoundProperty(evaluator, bar(a));

        REQUIRE(x.get() == 11);
    }

    SUBCASE("Function as makeBoundProperty argument")
    {
        Property<int> a{ 5 };
        Property<int> b{ 10 };

        // disambiguate the overloads of std::min
        const int &(*minFunc)(const int &, const int &) = &std::min<int>;
        auto x = makeBoundProperty(minFunc, a, b);

        REQUIRE(x.get() == 5);
    }

    SUBCASE("Function as makeBoundProperty argument with evaluator")
    {
        Property<int> a{ 5 };
        Property<int> b{ 10 };

        BindingEvaluator evaluator;
        auto x = makeBoundProperty(
                evaluator, [](const auto &a, const auto &b) { return std::max(a, b); }, a, b);

        REQUIRE(x.get() == 10);

        b = 2;

        REQUIRE(x.get() == 10);

        evaluator.evaluateAll();

        REQUIRE(x.get() == 5);
    }

    SUBCASE("Std binary function")
    {
        Property<int> a{ 5 };
        Property<int> b{ 7 };

        // max is declared as KDBINDINGS_DECLARE_STD_FUNCTION above.
        auto x = makeBoundProperty(max(a, b));

        REQUIRE(x.get() == 7);

        b = 2;
        REQUIRE(x.get() == 5);
    }
}

TEST_CASE("Binding evaluations")
{
    SUBCASE("A manual mode binding is not evaluated until requested")
    {
        auto evaluator = BindingEvaluator{};
        Property<int> a{ 8 };
        Property<int> b{ 7 };
        auto x = makeBoundProperty(evaluator, a + b);
        REQUIRE(x.get() == 15);

        a = 13;
        REQUIRE(x.get() == 15);

        evaluator.evaluateAll();
        REQUIRE(x.get() == 20);
    }

    SUBCASE("An immediate mode binding is evaluated immediately")
    {
        Property<int> a{ 8 };
        Property<int> b{ 7 };
        auto x = makeBoundProperty(a + b);
        REQUIRE(x.get() == 15);

        a = 13;
        REQUIRE(x.get() == 20);
    }
}
