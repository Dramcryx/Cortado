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

add_executable(main_example example/main_example.cpp "include/Cortado/Concepts/AtomicCompareExchange.h" "include/Cortado/Detail/CoroutinePromiseBase.h" "include/Cortado/Concepts/AtomicIncDec.h" "include/Cortado/Concepts/TaskImpl.h" "include/Cortado/Detail/AtomicRefCount.h" "include/Cortado/Task.h" "include/Cortado/Concepts/ErrorHandler.h" "include/Cortado/Await.h" "include/Cortado/Concepts/CoroutineScheduler.h" "include/Cortado/Concepts/BackgroundResumable.h" "include/Cortado/Common/STLExceptionHandler.h" "include/Cortado/Common/STLAtomicIncDec.h" "include/Cortado/Common/Win32CoroutineScheduler.h" "include/Cortado/Common/Win32AtomicCompareExchange.h")
target_include_directories(main_example PUBLIC ./include)
target_compile_definitions(main_example PUBLIC _AMD64_ _UNICODE)