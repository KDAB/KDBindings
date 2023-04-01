/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sean Harmer <sean.harmer@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include <kdbindings/node_functions.h>
#include <kdbindings/node.h>
#include <kdbindings/make_node.h>
#include <kdbindings/property.h>

#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

using namespace KDBindings;

static_assert(std::is_destructible<Private::NodeInterface<float>>{});
static_assert(!std::is_default_constructible<Private::Node<float>>{});
static_assert(!std::is_copy_constructible<Private::Node<float>>{});
static_assert(!std::is_copy_assignable<Private::Node<float>>{});
static_assert(std::is_nothrow_move_constructible<Private::Node<float>>{});
static_assert(std::is_nothrow_move_assignable<Private::Node<float>>{});

TEST_CASE("is_property")
{
    SUBCASE("resolves a Property<int> to true")
    {
        REQUIRE(Private::is_property<Property<int>>::value);
    }

    SUBCASE("resolves an int to false")
    {
        REQUIRE_FALSE(Private::is_property<int>::value);
    }
}

TEST_CASE("Expression node construction and basic evaluation")
{
    SUBCASE("create an expression node from a constant value")
    {
        auto n = Private::makeNode(7);
        REQUIRE_FALSE(n.isDirty());
        REQUIRE(n.evaluate() == 7);

        REQUIRE(n.evaluate() == 7);
    }

    SUBCASE("create an expression node from a property")
    {
        Property<int> property(8);
        auto n = Private::makeNode(property);

        REQUIRE(n.evaluate() == 8);
    }

    SUBCASE("an node wrapping a property is marked dirty when the property changes")
    {
        Property<int> property(8);
        auto n = Private::makeNode(property);

        property = 25;
        REQUIRE(n.isDirty());
    }

    SUBCASE("a change is reflected when the property changes")
    {
        Property<int> property(8);
        auto n = Private::makeNode(property);

        property = 25;
        REQUIRE(n.evaluate() == 25);
    }

    SUBCASE("deleting the property of a node throws an exception once evaluated")
    {
        auto *property = new Property<int>(8);

        auto node = Private::makeNode(*property);

        delete property;

        REQUIRE_THROWS_AS(node.evaluate(), PropertyDestroyedError);
    }

    SUBCASE("create an expression node from a unary function")
    {
        auto n = Private::makeNode([](int x) { return x * x; },
                                   Private::makeNode(5));

        REQUIRE(n.evaluate() == 25);
    }

    SUBCASE("a change is reflected in a unary function node")
    {
        Property<int> inputProperty(5);
        auto n = Private::makeNode([](auto x) { return x * x; },
                                   Private::makeNode(inputProperty));

        inputProperty = 7;
        REQUIRE(n.evaluate() == 49);
        REQUIRE_FALSE(n.isDirty());
    }

    SUBCASE("a change is not reflected in a unary function node without evaluate being called")
    {
        Property<int> inputProperty(5);
        auto n = Private::makeNode([](auto x) { return x * x; },
                                   Private::makeNode(inputProperty));

        inputProperty = 7;
        REQUIRE(n.isDirty());
    }

    SUBCASE("a binary function node can be evaluated")
    {
        auto n = Private::makeNode([](auto x, auto y) { return x * y; },
                                   Private::makeNode(3),
                                   Private::makeNode(6));

        REQUIRE(n.evaluate() == 18);
        REQUIRE_FALSE(n.isDirty());
    }

    SUBCASE("a change is reflected in a binary function node")
    {
        Property<int> width(3);
        Property<int> height(4);
        auto n = Private::makeNode([](auto x, auto y) { return x * y; },
                                   Private::makeNode(width),
                                   Private::makeNode(height));

        height = 7;
        n.evaluate();
        REQUIRE_FALSE(n.isDirty());
        REQUIRE(n.evaluate() == 21);
    }

    SUBCASE("a change is not reflected in a binary function node without evaluate being called")
    {
        Property<int> width(3);
        Property<int> height(4);
        auto n = Private::makeNode([](auto x, auto y) { return x * y; },
                                   Private::makeNode(width),
                                   Private::makeNode(height));

        height = 7;
        REQUIRE(n.isDirty());
    }
}

TEST_CASE("Expression trees are only evaluated if they are dirty")
{
    SUBCASE("a unary function node only evaluates function if dirty")
    {
        int callCount = 0;
        Property<int> inputProperty(5);
        auto n = Private::makeNode([&](auto x) {
            ++callCount;
            return x * x;
        },
                                   Private::makeNode(inputProperty));

        // The ctor evaluates the expression
        REQUIRE(callCount == 1);

        // Try to evaluate again, should use cached value
        REQUIRE(n.evaluate() == 25);
        REQUIRE(callCount == 1);

        // Now change the input value and re-evaluate. This should trigger another call
        inputProperty = 7;
        n.evaluate();
        REQUIRE(callCount == 2);
    }

    SUBCASE("a binary function node only evaluates function if dirty")
    {
        int callCount = 0;
        Property<int> width(3);
        Property<int> height(4);
        auto n = Private::makeNode([&](auto x, auto y) {
            ++callCount;
            return x * y;
        },
                                   Private::makeNode(width), Private::makeNode(height));

        // The ctor evaluates the expression
        REQUIRE(callCount == 1);

        // Try to evaluate again, should use cached value
        REQUIRE(n.evaluate() == 12);
        REQUIRE(callCount == 1);

        // Now change the input value and re-evaluate. This should trigger another call
        width = 5;
        height = 7;

        REQUIRE(callCount == 1);

        n.evaluate();
        REQUIRE(callCount == 2);
        REQUIRE(n.evaluate() == 35);
    }
}

TEST_CASE("Create more complex expression node trees")
{
    SUBCASE("y = 2 * (a + b)")
    {
        Property<int> a(3);
        Property<int> b(4);

        auto n = Private::makeNode([](auto x) { return 2 * x; },
                                   Private::makeNode([](auto x, auto y) { return x + y; },
                                                     Private::makeNode(a),
                                                     Private::makeNode(b)));

        REQUIRE(n.evaluate() == 14);
    }

    SUBCASE("y = 2 * (a + b)^2")
    {
        Property<int> a(3);
        Property<int> b(4);

        auto n = Private::makeNode([](auto x) { return 2 * x; },
                                   Private::makeNode([](auto x) { return x * x; },
                                                     Private::makeNode([](auto x, auto y) { return x + y; },
                                                                       Private::makeNode(a),
                                                                       Private::makeNode(b))));

        REQUIRE(n.evaluate() == 98);
    }
}

TEST_CASE("Moving nodes and properties")
{
    SUBCASE("a move constructed node wrapping a value can be evaluated")
    {
        auto n = Private::makeNode(7);
        auto movedNode{ std::move(n) };
        REQUIRE_FALSE(movedNode.isDirty());
        REQUIRE(movedNode.evaluate() == 7);
    }

    SUBCASE("a move assigned node wrapping a value can be evaluated")
    {
        auto n = Private::makeNode(7);
        auto movedNode = std::move(n);
        REQUIRE_FALSE(movedNode.isDirty());
        REQUIRE(movedNode.evaluate() == 7);
    }

    SUBCASE("a move constructed node wrapping a property can be evaluated")
    {
        Property<int> property{ 69 };
        auto n = Private::makeNode(property);
        auto movedNode{ std::move(n) };
        REQUIRE_FALSE(movedNode.isDirty());
        REQUIRE(movedNode.evaluate() == 69);
    }

    SUBCASE("a node wrapping a property can be evaluated following a move of the property")
    {
        Property<int> property{ 69 };
        auto n = Private::makeNode(property);
        Property<int> movedProperty = std::move(property);
        movedProperty = 75;

        REQUIRE(n.isDirty());
        REQUIRE(n.evaluate() == 75);
    }

    SUBCASE("a node referring to a property is invalidated when a property is moved into the referred one")
    {
        Property<int> property{ 69 };
        auto n = Private::makeNode(property);

        // this will destruct the original property and replace it with a new one,
        // breaking any existing connections it has.
        property = Property<int>(0);

        REQUIRE_FALSE(n.isDirty());
        REQUIRE_THROWS_AS(n.evaluate(), PropertyDestroyedError);
    }

    SUBCASE("a node wrapping a property can be evaluated following a move of both the node and property")
    {
        Property<int> property{ 69 };
        auto n = Private::makeNode(property);

        auto movedNode = std::move(n);
        Property<int> movedProperty = std::move(property);

        movedProperty = 75;
        REQUIRE(movedNode.isDirty());
        REQUIRE(movedNode.evaluate() == 75);
    }

    SUBCASE("a unary node can be evaluated following a move")
    {
        Property<int> inputProperty(5);
        auto n = Private::makeNode([](auto x) { return x * x; },
                                   Private::makeNode(inputProperty));

        auto movedNode = std::move(n);

        inputProperty = 7;
        REQUIRE(movedNode.isDirty());

        REQUIRE(movedNode.evaluate() == 49);
        REQUIRE_FALSE(movedNode.isDirty());
    }

    SUBCASE("a unary node can be evaluated following a move of the input property")
    {
        Property<int> property(5);
        auto n = Private::makeNode([](auto x) { return x * x; },
                                   Private::makeNode(property));

        Property<int> movedProperty = std::move(property);

        movedProperty = 7;
        REQUIRE(n.isDirty());

        REQUIRE(n.evaluate() == 49);
        REQUIRE_FALSE(n.isDirty());
    }

    SUBCASE("a unary node can be evaluated following a move of both the node and property")
    {
        Property<int> property(5);
        auto n = Private::makeNode([](auto x) { return x * x; },
                                   Private::makeNode(property));

        auto movedNode = std::move(n);
        Property<int> movedProperty = std::move(property);

        movedProperty = 7;
        REQUIRE(movedNode.isDirty());

        REQUIRE(movedNode.evaluate() == 49);
        REQUIRE_FALSE(movedNode.isDirty());
    }
}

TEST_CASE("bindable_value_type and operator_node")
{
    SUBCASE("resolves the type contained in a property")
    {
        REQUIRE(
                std::is_same_v<
                        Private::bindable_value_type<Property<int>>::type,
                        int>);
    }

    SUBCASE("resolves the type contained in an expression node")
    {
        REQUIRE(
                std::is_same_v<
                        Private::bindable_value_type<Private::Node<int>>::type,
                        int>);
    }

    SUBCASE("resolves the type returned by an operator expression node")
    {
        auto s = [](int x, int y) { return x + y; };
        REQUIRE(
                std::is_same_v<
                        Private::operator_node_result<decltype(s), Property<int>, Property<int>>::type,
                        int>);

        auto t = [](float x) { return x * x; };
        REQUIRE(
                std::is_same_v<
                        Private::operator_node_result<decltype(t), Property<float>>::type,
                        float>);

        auto u = [](float x) { return double(x * x); };
        REQUIRE(
                std::is_same_v<
                        Private::operator_node_result<decltype(u), Property<float>>::type,
                        double>);
    }
}
#include <kdbindings/node_operators.h>

#define KDBINDINGS_UNARY_OPERATOR_TEST(OP, TYPE, VALUE) \
    SUBCASE(#OP)                                        \
    {                                                   \
        Property<TYPE> property(VALUE);                 \
        auto node = OP property;                        \
                                                        \
        REQUIRE(node.evaluate() == (OP(VALUE)));        \
                                                        \
        auto node2 = OP std::move(node);                \
        REQUIRE(node2.evaluate() == (OP(OP(VALUE))));   \
    }

TEST_CASE("Test unary operators")
{
    KDBINDINGS_UNARY_OPERATOR_TEST(!, bool, true)

    KDBINDINGS_UNARY_OPERATOR_TEST(~, uint8_t, 25)

    KDBINDINGS_UNARY_OPERATOR_TEST(+, int, -10)

    KDBINDINGS_UNARY_OPERATOR_TEST(-, int, 10)
}

#define KDBINDINGS_BINARY_OPERATOR_TEST(OP, TYPE, VALUE, OTHER)     \
    SUBCASE(#OP)                                                    \
    {                                                               \
        Property<TYPE> property(VALUE);                             \
                                                                    \
        auto node = property OP(OTHER);                             \
        REQUIRE(node.evaluate() == ((VALUE)OP(OTHER)));             \
                                                                    \
        auto node2 = std::move(node) OP(OTHER);                     \
        REQUIRE(node2.evaluate() == (((VALUE)OP(OTHER))OP(OTHER))); \
    }

#define KDBINDINGS_COMPARISON_OPERATOR_TEST(OP, TYPE, VALUE, OTHER)                    \
    SUBCASE(#OP)                                                                       \
    {                                                                                  \
        Property<TYPE> property(VALUE);                                                \
                                                                                       \
        auto node = property OP(OTHER);                                                \
        REQUIRE(node.evaluate() == ((VALUE)OP(OTHER)));                                \
                                                                                       \
        /* the + unary operator typically doesn't do anything, but it creates a Node*/ \
        auto node2 = +property OP(OTHER);                                              \
        REQUIRE(node2.evaluate() == (+(VALUE)OP(OTHER)));                              \
    }

TEST_CASE("Test binary operators")
{
    KDBINDINGS_BINARY_OPERATOR_TEST(*, int, 5, 2);
    KDBINDINGS_BINARY_OPERATOR_TEST(/, int, 8, 2);
    KDBINDINGS_BINARY_OPERATOR_TEST(%, int, 8, 2);
    KDBINDINGS_BINARY_OPERATOR_TEST(+, int, 8, 2);

    KDBINDINGS_BINARY_OPERATOR_TEST(-, int, 8, 2);

    KDBINDINGS_BINARY_OPERATOR_TEST(<<, int, 12, 2);
    KDBINDINGS_BINARY_OPERATOR_TEST(>>, int, 12, 2);

    KDBINDINGS_BINARY_OPERATOR_TEST(&, int, 12, 2);
    KDBINDINGS_BINARY_OPERATOR_TEST(|, int, 12, 2);
    KDBINDINGS_BINARY_OPERATOR_TEST(^, int, 12, 2);

    KDBINDINGS_BINARY_OPERATOR_TEST(==, bool, true, false);
    KDBINDINGS_BINARY_OPERATOR_TEST(!=, bool, true, false);

    KDBINDINGS_BINARY_OPERATOR_TEST(&&, bool, true, false);
    KDBINDINGS_BINARY_OPERATOR_TEST(||, bool, false, true);

    KDBINDINGS_COMPARISON_OPERATOR_TEST(<, int, 5, 2);
    KDBINDINGS_COMPARISON_OPERATOR_TEST(>, int, 5, 2);
    KDBINDINGS_COMPARISON_OPERATOR_TEST(<=, int, 5, 2);
    KDBINDINGS_COMPARISON_OPERATOR_TEST(>=, int, 5, 2);
}

#define KDBINDINGS_NODE_FUNCTION_TEST(NAME, TYPE, VALUE, RESULT) \
    SUBCASE(#NAME)                                               \
    {                                                            \
        Property<TYPE> property(VALUE);                          \
                                                                 \
        auto node = NAME(property);                              \
        REQUIRE(node.evaluate() == (RESULT));                    \
    }

TEST_CASE("Test node functions")
{
    KDBINDINGS_NODE_FUNCTION_TEST(floor, float, 50.2f, 50.f);
    KDBINDINGS_NODE_FUNCTION_TEST(ceil, float, 50.2f, 51.f);

    KDBINDINGS_NODE_FUNCTION_TEST(sin, float, 0.f, 0.f);
    KDBINDINGS_NODE_FUNCTION_TEST(cos, float, 0.f, 1.f);
    KDBINDINGS_NODE_FUNCTION_TEST(tan, float, 0.f, 0.f);

    KDBINDINGS_NODE_FUNCTION_TEST(asin, float, 0.f, 0.f);
    KDBINDINGS_NODE_FUNCTION_TEST(acos, float, 1.f, 0.f);
    KDBINDINGS_NODE_FUNCTION_TEST(atan, float, 0.f, 0.f);
}
