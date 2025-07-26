# CMakeList.txt: файл проекта CMake верхнего уровня; проведите глобальную конфигурацию
# и включите сюда подпроекты.
#
cmake_minimum_required (VERSION 3.8)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Cortado")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (WIN32)
  add_executable(native_example "example/exampleWin32.cpp" "include/Cortado/Concepts/Atomic.h")
  target_include_directories(native_example PUBLIC ./include)
  target_compile_definitions(native_example PUBLIC _AMD64_ _UNICODE)
endif()

if (APPLE)
  add_executable(native_example "example/exampleMac.cpp")
  target_include_directories(native_example PUBLIC ./include)
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0 # Use a specific release tag or commit hash
)

FetchContent_MakeAvailable(googletest)

include(tests/CMakeLists.txt)
