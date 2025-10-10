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

add_library(Cortado INTERFACE)
target_include_directories(Cortado INTERFACE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(DIRECTORY include/Cortado DESTINATION include)
install(TARGETS Cortado EXPORT CortadoTargets)
install(EXPORT CortadoTargets
    FILE CortadoTargets.cmake
    NAMESPACE Cortado::
    DESTINATION lib/cmake/Cortado)

# Install LLDB scripts
install(
    DIRECTORY tools/lldb/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/lldb
    FILES_MATCHING PATTERN "*.py"
)

# Native examples
include(example/CMakeLists.txt)

# Doxygen generator
include(doc/CMakeLists.txt)

# Tests
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    include(tests/CMakeLists.txt)
endif()
