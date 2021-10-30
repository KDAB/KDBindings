/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Leon Matthes <leon.matthes@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/
#include <kdbindings/utils.h>
#include <ostream>
#include <type_traits>

using namespace KDBindings::Private;

// Test free function arity
int function(int, int);
int functionNoexcept(int, int) noexcept;

static_assert(get_arity(function) == 2);
static_assert(get_arity(functionNoexcept) == 2);

// Test member function arity
class GetArityTest
{
public:
    int member(int, int);
    void memberConst(int) const;
    bool memberVolatile() volatile;
    void memberRef(int, int) const &;
    void memberRRef(int, int) &&;

    int memberNoexcept(int, int) noexcept;
    void memberConstNoexcept(int) const noexcept;
    bool memberVolatileNoexcept() volatile noexcept;
    void memberRefNoexcept(int, int) const &noexcept;
    void memberRRefNoexcept(int, int) &&noexcept;

    // GetArityTest is also a callable object
    void operator()(int, int) const;
};

static_assert(get_arity(&GetArityTest::member) == 3);
static_assert(get_arity(&GetArityTest::memberConst) == 2);
static_assert(get_arity(&GetArityTest::memberVolatile) == 1);
static_assert(get_arity(&GetArityTest::memberRef) == 3);
static_assert(get_arity(&GetArityTest::memberRRef) == 3);

static_assert(get_arity(&GetArityTest::memberNoexcept) == 3);
static_assert(get_arity(&GetArityTest::memberConstNoexcept) == 2);
static_assert(get_arity(&GetArityTest::memberVolatileNoexcept) == 1);
static_assert(get_arity(&GetArityTest::memberRefNoexcept) == 3);
static_assert(get_arity(&GetArityTest::memberRRefNoexcept) == 3);

// Test arity of a generic callable object
static_assert(get_arity<GetArityTest>() == 2);

// Test arity with noexcept(...) syntax
template<typename T>
class GetArityTemplateTest
{
public:
    T memberOptionalNoexcept(T, T) noexcept(std::is_nothrow_default_constructible_v<T>);
};

// int constructor is noexcept
static_assert(get_arity(&GetArityTemplateTest<int>::memberOptionalNoexcept) == 3);
// std::ostream constructor is not noexcept
static_assert(get_arity(&GetArityTemplateTest<std::ostream>::memberOptionalNoexcept) == 3);

// Test lambda arity
static_assert(get_arity([]() {}) == 0);
static_assert(get_arity([](int, int) {}) == 2);
static_assert(get_arity([](int, int) { return false; }) == 2);
