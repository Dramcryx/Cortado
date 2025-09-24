/// @file exampleWin32.cpp
/// Usage example for Cortado library on Win32.
///

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/AsyncMutex.h>
#include <Cortado/Common/Win32Mutex.h>

// STL
//
#include <iostream>

using namespace Cortado;
using namespace Cortado::Common;

using MutexT = AsyncMutex<std::atomic_ulong, Win32Mutex>;

class WithAsyncMethod
{
public:
    Task<int> VoidAsync()
    {
        co_return 1;
    }
};

Task<int> ReturnFromBackgroundThread()
{
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";

    co_await ResumeBackground();

    std::cout << __FUNCTION__ << " Resumed on thread " << GetCurrentThreadId() << "\n";

    co_return 42;
}

Task<> WhenAllBackgroundTasks()
{
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";

    Task<int> tasks[]
    {
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread(),
        WithAsyncMethod{}.VoidAsync()
    };

    co_await WhenAll(tasks[0], tasks[1], tasks[2], tasks[3]);

    std::cout << __FUNCTION__ << " Resumed on thread " << GetCurrentThreadId() << "\n";

    co_return;
}

Task<> WhenAnyBackgroundTask()
{
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";

    co_await WhenAllBackgroundTasks();

    Task<int> tasks[]
    {
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread(),
        ReturnFromBackgroundThread()
    };

    co_await WhenAny(tasks[0], tasks[1], tasks[2]);

    std::cout << __FUNCTION__ << " Resumed on thread " << GetCurrentThreadId() << "\n";

    co_return;
}

Task<int> AsyncMutexBackgroudContention()
{
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";

    co_await WhenAnyBackgroundTask();

    MutexT mutex;

    int count = 0;

    auto task = [&] () -> Task<void>
    {
        co_await ResumeBackground();

        std::this_thread::sleep_for(std::chrono::seconds(rand() % 1));

        co_await mutex.ScopedLockAsync();

        ++count;
    };

    Task<void> tasks[]{ task(), task() };

    co_await WhenAll(tasks[0], tasks[1]);

    co_return count;
}

Task<int> AsyncMutexBackgroundContentionManual()
{
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";

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
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";

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
    // Enable memory leak checking at program exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    std::cout << std::boolalpha << (AsyncMutexParentWithChildContention().Get() == 6) << "\n";
    return 0;
}