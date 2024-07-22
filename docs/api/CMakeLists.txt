# This file is part of KDBindings.
#
# SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group
# company <info@kdab.com> Author: Allen Winter <allen.winter@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

# dot should come with Doxygen find_package(Doxygen)
if(DOXYGEN_DOT_EXECUTABLE)
    set(HAVE_DOT "YES")
else()
    # cmake-lint: disable=C0301
    set(HAVE_DOT "NO")
    message(
        STATUS
            "Unable to provide inheritance diagrams for the API documentation. To fix, install the graphviz project from https://www.graphviz.org"
    )
endif()

file(GLOB _dox_deps *.dox *.html)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

# apidox generation using doxygen
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

add_custom_target(
    docs
    # Execute Doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs ${DOXYGEN_OUTPUT_DIR}/mkdocs
    # copy some files by-hand that are referred to by the markdown README.
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/LICENSES/MIT.txt
            ${DOXYGEN_OUTPUT_DIR}/mkdocs/docs/license.md
    DEPENDS ${_dox_deps} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building Documentation"
)

# The tags file is only created when 'make docs' is run first
if(EXISTS "${DOXYGEN_OUTPUT_DIR}/kdbindings.tags")
    install(FILES ${DOXYGEN_OUTPUT_DIR}/kdbindings.tags DESTINATION ${INSTALL_DOC_DIR})
endif()
