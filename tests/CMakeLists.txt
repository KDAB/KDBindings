# This file is part of KDBindings.
#
# SPDX-FileCopyrightText: 2021-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Sean Harmer <sean.harmer@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

cmake_minimum_required(VERSION 3.12)
project(KDBindings-Tests)

include_directories(./doctest)

add_subdirectory(binding)
add_subdirectory(node)
add_subdirectory(property)
add_subdirectory(signal)
add_subdirectory(utils)
