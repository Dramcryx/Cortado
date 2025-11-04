add_executable(example example/example.h example/example.cpp)

target_include_directories(example PRIVATE ./include)

if (WIN32)
  target_compile_definitions(example PRIVATE _AMD64_ _UNICODE)
  target_link_libraries(example PRIVATE Synchronization.lib)
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
    target_compile_options(example PRIVATE -fsanitize=address)
    target_link_options(example PRIVATE -fsanitize=address)
  endif()

  if (TSAN)
    if (ASAN)
      message(FATAL_ERROR "Only one sanitizer at a time is allowed!")
    endif()
    message(STATUS "Building with -fsanitize=thread")
    target_compile_options(example PRIVATE -fsanitize=thread)
    target_link_options(example PRIVATE -fsanitize=thread)
  endif()
else()
  message(STATUS ${CMAKE_CXX_COMPILER_ID})
endif()
