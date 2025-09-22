include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0 # Use a specific release tag or commit hash
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(CortadoTests ${CMAKE_CURRENT_LIST_DIR}/DefaultTaskTests.cpp ${CMAKE_CURRENT_LIST_DIR}/DefaultTaskWithAdditionalStorageTests.cpp)

target_link_libraries(CortadoTests PRIVATE GTest::gtest_main GTest::gtest)
target_include_directories(CortadoTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (MSVC)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
            target_compile_definitions(CortadoTests PRIVATE _AMD64_)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$")
            target_compile_definitions(CortadoTests PRIVATE _ARM64_)
        endif()
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i[3-6]86)$")
            target_compile_definitions(CortadoTests PRIVATE _X86_)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM|armv7)$")
            target_compile_definitions(CortadoTests PRIVATE _ARM_)
        endif()
    endif()
    foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
        if(${flag_var} MATCHES "/MDd")
            string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
        endif()
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

add_test(NAME CortadoTests COMMAND CortadoTests)

include(GoogleTest)
gtest_discover_tests(CortadoTests)
