# This file is part of KDBindings.
#
# SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Sean Harmer <sean.harmer@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

cmake_minimum_required(VERSION 3.12)
project(KDBindings-Tests)

# We use `SYSTEM` here, because we don't want to see warnings from doctest in clang-tidy.
# See: https://www.reddit.com/r/cmake/comments/zhqq9f/comment/j34m17q/?utm_source=share&utm_medium=web2x&context=3
include_directories(SYSTEM ./doctest)

add_subdirectory(binding)
add_subdirectory(node)
add_subdirectory(property)
add_subdirectory(signal)
add_subdirectory(utils)
