# This file is part of KDBindings.
#
# SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Leon Matthes <leon.matthes@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

project(test-utils VERSION 0.1 LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    tst_gen_index_array.cpp
    tst_get_arity.cpp
    tst_utils_main.cpp
)
target_link_libraries(${PROJECT_NAME} KDAB::KDBindings)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
