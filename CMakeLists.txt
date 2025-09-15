cmake_minimum_required (VERSION 3.10)

project ("Cortado")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find clang-format
find_program(CLANG_FORMAT_EXE 
    NAMES "clang-format"
    DOC "Path to clang-format executable"
)

if(NOT CLANG_FORMAT_EXE)
    message(STATUS "clang-format not found.")
else()
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
    
    # Get all project files
    file(GLOB_RECURSE ALL_SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_SOURCE_DIR}/include/*.h
    )

    # Create format target
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE}
        -i
        -style=file
        ${ALL_SOURCE_FILES}
        COMMENT "Auto formatting of all source files"
    )

    # Create format-check target (doesn't modify files, just checks)
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE}
        --dry-run
        --Werror
        -style=file
        ${ALL_SOURCE_FILES}
        COMMENT "Checking format compliance"
    )
endif()

# Native examples
include(example/CMakeLists.txt)

# Doxygen generator
include(doc/CMakeLists.txt)

# Tests
include(tests/CMakeLists.txt)
