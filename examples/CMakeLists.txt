add_executable(ExampleDefaultTaskImpl examples/ExampleDefaultTaskImpl.cpp)

target_include_directories(ExampleDefaultTaskImpl PRIVATE ./include)

add_executable(ExampleCustomAllocator examples/ExampleCustomAllocator.cpp)

target_include_directories(ExampleCustomAllocator PRIVATE ./include)

add_executable(ExampleCustomScheduler examples/ExampleCustomScheduler.cpp)

target_include_directories(ExampleCustomScheduler PRIVATE ./include)

if (WIN32)
  target_compile_definitions(ExampleDefaultTaskImpl PRIVATE _AMD64_ _UNICODE)
  target_link_libraries(ExampleDefaultTaskImpl PRIVATE Synchronization.lib)

  target_compile_definitions(ExampleCustomAllocator PRIVATE _AMD64_ _UNICODE)
  target_link_libraries(ExampleCustomAllocator PRIVATE Synchronization.lib)

  target_compile_definitions(ExampleCustomScheduler PRIVATE _AMD64_ _UNICODE)
  target_link_libraries(ExampleCustomScheduler PRIVATE Synchronization.lib)
endif()

option(ASAN "Build with address sanitizer" OFF)
option(TSAN "Build with thread sanitizer" OFF)

# For clang, optionally build with sanitizers
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if (ASAN AND TSAN)
    message(FATAL_ERROR "Only one sanitizer at a time is allowed!")
  endif()

  if (ASAN)
    message(STATUS "Building with -fsanitize=address")

    target_compile_options(ExampleDefaultTaskImpl PRIVATE -fsanitize=address)
    target_link_options(ExampleDefaultTaskImpl PRIVATE -fsanitize=address)

    target_compile_options(ExampleCustomAllocator PRIVATE -fsanitize=address)
    target_link_options(ExampleCustomAllocator PRIVATE -fsanitize=address)

    target_compile_options(ExampleCustomScheduler PRIVATE -fsanitize=address)
    target_link_options(ExampleCustomScheduler PRIVATE -fsanitize=address)
  endif()

  if (TSAN)
    message(STATUS "Building with -fsanitize=thread")
    target_compile_options(ExampleDefaultTaskImpl PRIVATE -fsanitize=thread)
    target_link_options(ExampleDefaultTaskImpl PRIVATE -fsanitize=thread)

    target_compile_options(ExampleCustomAllocator PRIVATE -fsanitize=address)
    target_link_options(ExampleCustomAllocator PRIVATE -fsanitize=address)

    target_compile_options(ExampleCustomScheduler PRIVATE -fsanitize=address)
    target_link_options(ExampleCustomScheduler PRIVATE -fsanitize=address)
  endif()
else()
  message(STATUS ${CMAKE_CXX_COMPILER_ID})
endif()
