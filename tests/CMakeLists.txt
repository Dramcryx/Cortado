
enable_testing()

add_executable(CortadoTests ${CMAKE_CURRENT_LIST_DIR}/DefaultTaskTests.cpp ${CMAKE_CURRENT_LIST_DIR}/DefaultTaskWithAdditionalStorageTests.cpp)

target_link_libraries(CortadoTests PRIVATE GTest::gtest_main GTest::gtest)
target_include_directories(CortadoTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_test(NAME CortadoTests COMMAND CortadoTests)

include(GoogleTest)
gtest_discover_tests(CortadoTests)
