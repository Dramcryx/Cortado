add_executable(native_example)

target_include_directories(native_example PRIVATE ./include)

if (WIN32)
  target_sources(native_example PRIVATE "example/exampleWin32.cpp")
  target_compile_definitions(native_example PRIVATE _AMD64_ _UNICODE)
  target_link_libraries(native_example PRIVATE Synchronization.lib)
endif()

if (APPLE)
  target_sources(native_example PRIVATE "example/exampleMacOS.cpp")
endif()

if (LINUX)
  target_sources(native_example PRIVATE "example/exampleLinux.cpp")
endif()

option(ASAN "Build with address sanitizer" OFF)
option(TSAN "Build with thread sanitizer" OFF)

# For clang, optionally build with sanitizers
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if (ASAN)
    if (TSAN)
      message(FATAL_ERROR "Only one sanitizer at a time is allowed!")
    endif()
    message(STATUS "Building with -fsanitize=address")
    target_compile_options(native_example PRIVATE -fsanitize=address)
    target_link_options(native_example PRIVATE -fsanitize=address)
  endif()

  if (TSAN)
    if (ASAN)
      message(FATAL_ERROR "Only one sanitizer at a time is allowed!")
    endif()
    message(STATUS "Building with -fsanitize=thread")
    target_compile_options(native_example PRIVATE -fsanitize=thread)
    target_link_options(native_example PRIVATE -fsanitize=thread)
  endif()
else()
  message(STATUS ${CMAKE_CXX_COMPILER_ID})
endif()
