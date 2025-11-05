cmake_minimum_required (VERSION 3.10)

project ("Cortado" VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find clang-format
find_program(CLANG_FORMAT_EXE 
    NAMES "clang-format"
    DOC "Path to clang-format executable"
)

# Enable clang-format as a custom target if present
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
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    include(tests/CMakeLists.txt)
endif()

# Packaging commons

# 1. Specify where public headers live
set(CORTADO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 2. Define an INTERFACE library (header-only)
add_library(Cortado INTERFACE)
target_include_directories(Cortado INTERFACE
    $<BUILD_INTERFACE:${CORTADO_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(Cortado INTERFACE cxx_std_20)

# 3. Install headers
install(
    DIRECTORY "${CORTADO_INCLUDE_DIR}/"
    DESTINATION include
)

# 4. Install the target (for find_package / CMake configs)
install(
    TARGETS Cortado
    EXPORT CortadoTargets
)

# 5. Install the CMake package config files
include(CMakePackageConfigHelpers)

# Create version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CortadoConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Create config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CortadoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CortadoConfig.cmake"
    INSTALL_DESTINATION lib/cmake/Cortado
)

# Export CortadoTargets file
install(
    EXPORT CortadoTargets
    FILE CortadoTargets.cmake
    NAMESPACE Cortado::
    DESTINATION lib/cmake/Cortado
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CortadoConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CortadoConfigVersion.cmake"
    DESTINATION lib/cmake/Cortado
)

install(FILES nuget/Cortado.targets DESTINATION build/native)

# CPack configuration

# 1. Set packaging metadata
set(CPACK_PACKAGE_NAME "Cortado")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "jopa19991@gmail.com")

# 2. Enable NuGet packaging
set(CPACK_GENERATOR "NuGet")
set(CPACK_NUGET_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_NUGET_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
set(CPACK_NUGET_PACKAGE_OWNERS "Maksim Vlasov")
set(CPACK_NUGET_PACKAGE_DESCRIPTION "Coroutine library for projects with strong legacy or limited STL usage.")
set(CPACK_NUGET_PACKAGE_LICENSEURL "https://opensource.org/licenses/MIT")
set(CPACK_NUGET_PACKAGE_REPOSITORY_URL "https://github.com/Dramcryx/Cortado")

# 3. Enable DEB/RPM packaging
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "jopa19991@gmail.com")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")

include(CPack)

# Install LLDB scripts
# install(
#     DIRECTORY tools/lldb/
#     DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/lldb
#     FILES_MATCHING PATTERN "*.py"
# )
