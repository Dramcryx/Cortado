/// @file ExampleDefaultTaskImpl.cpp
/// Usage example for Cortado library with default task implementation.
///

#include "ExampleDefaultTaskImpl.h"

// Cortado
//
#include <Cortado/AsyncMutex.h>
#include <Cortado/DefaultMutex.h>

// STL
//
#include <array>
#include <format>
#include <iostream>
#include <thread>

using namespace Cortado;
using namespace Cortado::Common;

using MutexT = AsyncMutex<std::atomic_int64_t, DefaultMutex>;

class WithAsyncMethod
{
public:
    Task<int> VoidAsync()
    {
        co_return 1;
    }
};

inline void LogStart(std::string_view funcName)
{
    constexpr std::string_view format{"[{}] Started on thread {}\n"};
    std::array<char, 128> buffer;
    auto [_, size] = std::format_to_n(buffer.data(),
                                      buffer.size(),
                                      format,
                                      funcName,
                                      THREAD_ID);

    std::cout << std::string_view{buffer.data(), (std::size_t)size};
}

inline void LogResumption(std::string_view funcName)
{
    constexpr std::string_view format{"[{}] Resumed on thread {}\n"};
    std::array<char, 128> buffer;
    auto [_, size] = std::format_to_n(buffer.data(),
                                      buffer.size(),
                                      format,
                                      funcName,
                                      THREAD_ID);

    std::cout << std::string_view{buffer.data(), (std::size_t)size};
}

Task<int> ReturnFromBackgroundThread()
{
    LogStart(__FUNCTION__);

    co_await ResumeBackground();

    LogResumption(__FUNCTION__);

    co_return 42;
}

Task<> WhenAllBackgroundTasks()
{
    LogStart(__FUNCTION__);

    Task<int> tasks[]
    {
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread(),
        WithAsyncMethod{}.VoidAsync()
    };

    co_await WhenAll(tasks[0], tasks[1], tasks[2], tasks[3]);

    LogResumption(__FUNCTION__);

    co_return;
}

Task<> WhenAnyBackgroundTask()
{
    LogStart(__FUNCTION__);

    co_await WhenAllBackgroundTasks();

    Task<int> tasks[]
    {
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread()
    };

    co_await WhenAny(tasks[0], tasks[1], tasks[2]);

    LogResumption(__FUNCTION__);

    co_return;
}

Task<int> AsyncMutexBackgroudContention()
{
    LogStart(__FUNCTION__);

    co_await WhenAnyBackgroundTask();

    MutexT mutex;

    int count = 0;

    auto task = [&] () -> Task<void>
    {
        co_await ResumeBackground();

        std::this_thread::sleep_for(std::chrono::seconds(rand() % 1));

        auto lock = co_await mutex.ScopedLockAsync();

        ++count;
    };

    Task<void> tasks[]{ task(), task() };

    co_await WhenAll(tasks[0], tasks[1]);

    co_return count;
}

Task<int> AsyncMutexBackgroundContentionManual()
{
    LogStart(__FUNCTION__);

    int count = co_await AsyncMutexBackgroudContention();

    MutexT mutex;

    auto task = [&] () -> Task<void>
    {
        co_await ResumeBackground();

        std::this_thread::sleep_for(std::chrono::seconds(rand() % 1));

        co_await mutex.LockAsync();

        ++count;

        mutex.Unlock();
    };

    Task<void> tasks[]{ task(), task() };

    co_await WhenAll(tasks[0], tasks[1]);

    co_return count;
}

Task<int> AsyncMutexParentWithChildContention()
{
    LogStart(__FUNCTION__);

    int count = co_await AsyncMutexBackgroundContentionManual();

    MutexT mutex;

    co_await mutex.LockAsync();

    auto task = [&] () -> Task<void>
    {
        co_await mutex.LockAsync();

        ++count;

        mutex.Unlock();
    };

    Task<void> tasks[]{ task(), task() };

    mutex.Unlock();

    co_await WhenAll(tasks[0], tasks[1]);

    co_return count;
}

int main()
{
#ifdef _WIN32
    // Enable memory leak checking at program exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // _WIN32

    std::cout << std::boolalpha << (AsyncMutexParentWithChildContention().Get() == 6) << "\n";
    return 0;
}
