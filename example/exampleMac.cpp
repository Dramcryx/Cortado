// Cortado
//
#include <Cortado/AsyncMutex.h>
#include <Cortado/Common/MacOSMutex.h>

// STL
//
#include <iostream>

using namespace Cortado;

Task<int> Ans()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    co_await Cortado::ResumeBackground();
    std::cout << __FUNCTION__ << " Resumed on thread " << pthread_self() << "\n";
    co_return 42;
}

Task<> Ans2()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    Task<int> tasks[]{ Ans(), Ans(), Ans() };
    co_await Cortado::WhenAll(tasks[0], tasks[1], tasks[2]);
    std::cout << __FUNCTION__ << " Resumed on thread " << pthread_self() << "\n";
    co_return;
}

Task<> Ans3()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    co_await Ans2();
    Task<int> tasks[]{ Ans(), Ans(), Ans() };
    co_await Cortado::WhenAny(tasks[0], tasks[1], tasks[2]);
    std::cout << __FUNCTION__ << " Resumed on thread " << pthread_self() << "\n";
    co_return;
}

Task<int> Ans4()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    co_await Ans3();

    AsyncMutex<std::atomic_ulong, Common::MacOSMutex> mutex;

    int count = 0;
    auto task = [&] () -> Task<void>
    {
        co_await Cortado::ResumeBackground();

        sleep(rand() % 1);
        co_await mutex.ScopedLockAsync();
        ++count;
    };

    Task<void> tasks[]{ task(), task() };
    co_await Cortado::WhenAll(tasks[0], tasks[1]);
    co_return count;
}

Task<int> Ans5()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    int count = co_await Ans4();

    AsyncMutex<std::atomic_ulong, Common::MacOSMutex> mutex;

    auto task = [&] () -> Task<void>
    {
        co_await Cortado::ResumeBackground();

        sleep(rand() % 1);
        co_await mutex.LockAsync();
        ++count;
        mutex.Unlock();
    };

    Task<void> tasks[]{ task(), task() };
    co_await Cortado::WhenAll(tasks[0], tasks[1]);
    co_return count;
}

Task<int> Ans6()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    int count = co_await Ans5();

    AsyncMutex<std::atomic_ulong, Common::MacOSMutex> mutex;

    co_await mutex.LockAsync();

    auto task = [&] () -> Task<void>
    {
        co_await mutex.LockAsync();
        ++count;
        mutex.Unlock();
    };

    Task<void> tasks[]{ task(), task() };

    mutex.Unlock();

    co_await Cortado::WhenAll(tasks[0], tasks[1]);
    co_return count;
}

int main()
{
    std::cout << std::boolalpha << (Ans6().Get() == 6) << "\n";
    return 0;
}