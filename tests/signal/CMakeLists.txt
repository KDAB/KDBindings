# This file is part of KDBindings.
#
# SPDX-FileCopyrightText: 2021-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Sean Harmer <sean.harmer@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

project(test-signal VERSION 0.1 LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    tst_signal.cpp
)

target_link_libraries(${PROJECT_NAME} KDAB::KDBindings)

# For some reason, CMake with gcc doesn't automatically include the pthread library
# when using std::thread. This is a workaround for that.
# IMHO, this is ridiculous, std::thread is part of the standard library, it should just work
# when I use C++, but it is what it is.
# See: https://cmake.cmake.narkive.com/wWDhK9RQ/undefined-reference-to-pthread-create
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_package (Threads)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()

add_test(${PROJECT_NAME} ${PROJECT_NAME})
