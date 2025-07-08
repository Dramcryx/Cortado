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

if (WIN32)
  add_executable(native_example "example/exampleWin32.cpp")
  target_include_directories(native_example PUBLIC ./include)
  target_compile_definitions(native_example PUBLIC _AMD64_ _UNICODE)
endif()

if (APPLE)
  add_executable(native_example "example/exampleMac.cpp")
  target_include_directories(native_example PUBLIC ./include)
endif()
