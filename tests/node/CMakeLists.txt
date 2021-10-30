# This file is part of KDBindings.
#
# SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Sean Harmer <sean.harmer@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

project(test-node VERSION 0.1 LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    tst_node.cpp
)
target_link_libraries(${PROJECT_NAME} KDAB::KDBindings)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
