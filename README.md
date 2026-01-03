# Cortado — a C++ coroutines strategy

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build Status](https://github.com/Dramcryx/Cortado/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=master)](https://github.com/Dramcryx/Cortado/actions/workflows/cmake-multi-platform.yml)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()

A small, flexible Task-based coroutine helper designed for legacy and constrained C++ runtimes.
Cortado provides a minimal Task abstraction with pluggable allocators, exception handlers,
atomics and schedulers so you can adopt coroutines without changing your project's runtime.

---

Table of contents
- Introduction
- Highlights
- Quick examples
  - Minimal Task
  - Offload to background
  - Await another Task
  - Await multiple Tasks
- Customization
- Getting started
- Roadmap & TODO
- Contributing & License

Introduction
------------
C++ coroutines provide language-level primitives but leave the supporting library/runtime to you.
Cortado fills that gap with a small, opinionated Task type and a set of common awaiters,
while keeping customization simple so you can plug in your project's allocator, exception model,
atomic primitives or scheduler.

Highlights
----------
- Small Task abstraction for suspendable functions.
- Pluggable: allocator, atomic, exception handler and scheduler can be replaced.
- Common awaiters: resume to background, when_all, when_any, awaiting other Tasks.
- Designed for projects that can't or don't want to rely on the default STL runtime.
- Focused on clarity and portability rather than microbenchmarks.

Quick examples
--------------
Minimal Task
```c++
#include <Cortado/Task.h>

Cortado::Task<> DoNothing()
{
    co_return;
}

Cortado::Task<int> DoNothingWithValue()
{
    co_return 42;
}
```

Offload to background
```c++
#include <Cortado/Await.h>

Cortado::Task<int> DoNothingAsync()
{
    co_await Cortado::ResumeBackground(); // 1) start on main thread
    co_return 42;                         // 2) resumes in a different thread
}

int main()
{
    int value = DoNothingAsync().Get(); // 42
    return 0;
}
```

Await another Task
```c++
#include <Cortado/Await.h>

Cortado::Task<int> AsyncAction()
{
    int subResult = co_await DoNothingAsync(); // suspends until child finishes
    co_return subResult + 1;
}
```

Await multiple Tasks
```c++
#include <Cortado/Await.h>

Cortado::Task<int> AsyncAction()
{
    Cortado::Task<int> tasks[] = { DoNothingAsync(), DoNothingAsync(), DoNothingAsync() };

    co_await Cortado::WhenAll(tasks[0], tasks[1], tasks[2]);

    int sum = tasks[0].Get() + tasks[1].Get() + tasks[2].Get();
    co_return sum + 1;
}
```

Customization
---------------------------------------
In Cortado you can customize multiple core concepts of coroutine runtime. They include:
1) Allocator - it must follow `CoroutineAllocator` concept. A detailed exmaple is in `examples/ExampleCustomAllocator.cpp`.
2) Scheduler - it must follow `CoroutineScheduler` concept. A detailed example is in `examples/ExampleCustomScheduler.cpp`.
3) Exception handler:
```c++
// Implement your handler (no STL exception_ptr required)
class SillyExceptionHandler
{
public:
    using Exception = errno_t;

    static Exception Catch()    { return EACCES; }
    static void Rethrow(Exception e) { throw e; }
};

// Compose your Task implementation with custom pieces
struct SillyTaskImpl :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLCoroutineAllocator,
    SillyExceptionHandler,
    DefaultScheduler
{
    using Event = DefaultEvent;
};

template <typename T = void>
using Task = Cortado::Task<T, SillyTaskImpl>;
```
4) Atomic primitive which is required for task concurrency.
5) Event primitive which is also required for task concurrency and sync wait.

Getting started
---------------
To test the default implementation, it is required to run a platform with futex support. Fallback to kernel-mode primitives is TODO.

These systems include:
1) Windows 8+/Windows Server 2012+
2) macOS 14.4+
3) Linux with kernel 2.5.7+ (futex2 is also TODO)

CMake on any platform
---------------
If your project supports CMake, the best way it to do the same way as Google Test:
```cmake
include(FetchContent)

FetchContent_Declare(
    Cortado
    GIT_REPOSITORY https://github.com/Dramcryx/Cortado.git
)
FetchContent_MakeAvailable(Cortado)

add_executable(YOUR_TARGET main.cpp)

target_link_libraries(YOUR_TARGET PRIVATE Cortado)
```
Replace `YOUR_TARGET` with the actual target you are building. Cortado headers will be available since then.

Debian package (Debian, Ubuntu, Linux Mint etc.)
---------------
Download a deb package from releases: https://github.com/Dramcryx/Cortado/releases, then from terminal:
```
sudo dpkg -i Cortado-0.3.0.deb
```

RPM package (Fedora)
---------------
To be packed automatically. You can pack RPM manually using CPack:
```
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local/

cpack -G RPM ./build/CPackConfig.cmake
```
This will generate a file named Cortado-0.3.0.rpm.
Next, run rpm installation:
```
sudo rpm -i Cortado-0.3.0.rpm
```

NuGet package
---------------
In Visual Sutdio with your project open right-click on project -> `Manage NuGet Packages...` -> `Browse` -> type `Cortado` and install latest version.

Roadmap & TODO
--------------
- Proper packaging and releases.
- Better documentation and examples (including integration with various schedulers).
- Cancellation support.
- Fuzzer and tests.
- Stack tracing support (Natvis scripts, gdb/lldb helpers).
- MCS mutex implementation

Contributing
------------
Contributions welcome. Follow typical GitHub flow: fork, create topic branch, open a PR.
Keep changes focused and add tests/examples where appropriate.

License
-------
MIT — see LICENSE file.
