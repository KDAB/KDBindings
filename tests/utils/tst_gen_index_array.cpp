/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Leon Matthes <leon.matthes@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include <kdbindings/genindex_array.h>
#include <type_traits>
#include <set>

#include <doctest.h>

using namespace KDBindings::Private;

static_assert(std::is_nothrow_destructible<GenerationalIndexArray<int>>{});
static_assert(std::is_nothrow_default_constructible<GenerationalIndexArray<int>>{});
static_assert(std::is_copy_constructible<GenerationalIndexArray<int>>{});
static_assert(std::is_copy_assignable<GenerationalIndexArray<int>>{});
static_assert(std::is_nothrow_move_constructible<GenerationalIndexArray<int>>{});
static_assert(std::is_nothrow_move_assignable<GenerationalIndexArray<int>>{});

static_assert(std::is_nothrow_destructible<GenerationalIndex>{});
static_assert(std::is_nothrow_default_constructible<GenerationalIndex>{});
static_assert(std::is_nothrow_copy_constructible<GenerationalIndex>{});
static_assert(std::is_nothrow_copy_assignable<GenerationalIndex>{});
static_assert(std::is_nothrow_move_constructible<GenerationalIndex>{});
static_assert(std::is_nothrow_move_assignable<GenerationalIndex>{});

TEST_CASE("Construction")
{
    SUBCASE("A default constructed Array is empty")
    {
        GenerationalIndexArray<int> array;
        REQUIRE(array.entriesSize() == 0);
    }

    SUBCASE("A copy constructed array copies the values and keeps indices")
    {
        GenerationalIndexArray<int> array;
        auto index = array.insert(1);
        auto index2 = array.insert(2);
        array.erase(index);
        index = array.insert(3);

        auto secondArray = array;
        REQUIRE(array.entriesSize() == secondArray.entriesSize());
        REQUIRE(*array.get(index) == *secondArray.get(index));
        REQUIRE(*array.get(index2) == *secondArray.get(index2));
    }

    SUBCASE("A move constructed array empties the previous array")
    {
        GenerationalIndexArray<int> array;
        auto index = array.insert(1);
        auto index2 = array.insert(2);
        array.erase(index);
        index = array.insert(3);

        auto secondArray = std::move(array);
        REQUIRE(array.entriesSize() == 0);
        REQUIRE(array.get(index) == nullptr);
        REQUIRE(array.get(index2) == nullptr);

        REQUIRE(secondArray.entriesSize() == 2);
        REQUIRE(*secondArray.get(index) == 3);
        REQUIRE(*secondArray.get(index2) == 2);
    }
}

TEST_CASE("Insertion")
{
    SUBCASE("values can be inserted and retrieved")
    {
        GenerationalIndexArray<int> array;

        auto index = array.insert(5);
        auto index2 = array.insert(7);

        REQUIRE(array.entriesSize() == 2);
        REQUIRE(*array.get(index) == 5);
        REQUIRE(*array.get(index2) == 7);
    }
}

TEST_CASE("Deletion")
{
    SUBCASE("Deletion removes the value")
    {
        GenerationalIndexArray<int> array;

        auto index = array.insert(5);
        REQUIRE(array.entriesSize() == 1);

        array.erase(index);
        REQUIRE(array.get(index) == nullptr);
        REQUIRE_MESSAGE(array.entriesSize() == 1, "entriesSize doesn't get smaller during deletion");
    }

    SUBCASE("Deletion only invalidates the deleted index")
    {
        GenerationalIndexArray<int> array;

        auto index = array.insert(5);
        auto index2 = array.insert(7);
        auto index2Ptr = array.get(index2);

        array.erase(index);
        REQUIRE(array.get(index) == nullptr);
        REQUIRE(array.get(index2) == index2Ptr);
        REQUIRE(*array.get(index2) == 7);
    }

    SUBCASE("Clear invalidates all indices, but leaves capacity unchanged")
    {
        GenerationalIndexArray<int> array;

        auto index = array.insert(5);
        auto index2 = array.insert(7);

        array.clear();
        REQUIRE(array.entriesSize() == 2);
        REQUIRE(array.get(index) == nullptr);
        REQUIRE(array.get(index2) == nullptr);
    }

    SUBCASE("After clearing, spots in the array are reused")
    {
        GenerationalIndexArray<int> array;
        std::set<uint32_t> valueIndices;

        valueIndices.emplace(array.insert(5).index);
        valueIndices.emplace(array.insert(7).index);

        array.clear();

        std::set<uint32_t> newValueIndices;
        newValueIndices.emplace(array.insert(8).index);
        newValueIndices.emplace(array.insert(9).index);

        REQUIRE(array.entriesSize() == 2);
        REQUIRE(valueIndices == newValueIndices);
    }

    SUBCASE("After clearing, the generations are different")
    {
        GenerationalIndexArray<int> array;
        std::set<uint32_t> generations;

        generations.emplace(array.insert(5).generation);
        generations.emplace(array.insert(7).generation);

        array.clear();

        std::set<uint32_t> newGenerations;
        newGenerations.emplace(array.insert(8).generation);
        newGenerations.emplace(array.insert(9).generation);

        REQUIRE(array.entriesSize() == 2);
        for (const auto &generation : generations) {
            REQUIRE(newGenerations.find(generation) == newGenerations.end());
        }
    }
}

TEST_CASE("indexAtEntry")
{
    SUBCASE("An empty GenIndexArray never returns a valid index")
    {
        GenerationalIndexArray<int> array;

        for (uint32_t i = 0; i < 10; ++i) {
            REQUIRE_FALSE(array.indexAtEntry(i));
        }
    }

    SUBCASE("A full GenIndexArray returns a valid index for every entry, but not more")
    {
        GenerationalIndexArray<int> array;

        for (auto i = 0; i < 10; ++i) {
            array.insert(std::move(i));
        }

        for (uint32_t i = 0; i < array.entriesSize(); ++i) {
            REQUIRE(array.indexAtEntry(i));
            REQUIRE_FALSE(array.indexAtEntry(i + array.entriesSize()));
        }
    }
}
