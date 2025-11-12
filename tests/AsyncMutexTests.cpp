/// @file AsyncMutexTests.cpp
/// Tests for Cortado::AsyncMutex
///

#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/AsyncMutex.h>
#include <Cortado/DefaultEvent.h>
#include <Cortado/DefaultMutex.h>

template <typename T = void>
using Task = Cortado::Task<T>;

using AsyncMutex = Cortado::AsyncMutex<std::atomic_int64_t, Cortado::DefaultMutex>;

TEST(AsyncMutexTests, BasicLockUnlock)
{
    AsyncMutex mutex;

    auto task = [&]() -> Task<int>
    {
        auto mutexLock = co_await mutex.ScopedLockAsync();
        co_return 42;
    };

    // LockAsync does not really block, rather returns an awaiter
    // which will actually lock.
    //
    auto awaiter = mutex.LockAsync();

    // Expect succeeding on lock
    //
    EXPECT_TRUE(awaiter.await_ready());

    // Call task and expect it to suspend because we hold the lock from this thread
    //
    auto taskObject = task();

    // Call unlock and expect mutex to notify next waiter which is `taskObject`
    //
    mutex.Unlock();

    // The task must immediately return 42
    //
    EXPECT_TRUE(taskObject.IsReady());
}

TEST(AsyncMutexTests, BasicConcurrency)
{
    constexpr std::size_t ConcurrencyCount = 4;

    struct ConcurrencyRegister
    {
        std::atomic_int64_t SuspendedCount{0};
        Cortado::DefaultEvent SuspendAllReached;

        std::atomic_int64_t CurrentTaskIndex{0};
        Cortado::DefaultEvent PassedScopedLock[ConcurrencyCount];

    } static reg{};

    struct TaskImplWithSuspendedCounter :
        Cortado::Common::STLExceptionHandler,
        Cortado::Common::STLAtomic,
        Cortado::Common::STLCoroutineAllocator,
        Cortado::DefaultScheduler
    {
        struct AdditionalStorage
        {
            AdditionalStorage() : ThisTaskIndex{reg.CurrentTaskIndex++}
            {
            }

            std::int64_t ThisTaskIndex = 0;
        };

        using Event = Cortado::DefaultEvent;

        static void OnBeforeSuspend(AdditionalStorage &s)
        {
            if (++reg.SuspendedCount == ConcurrencyCount)
            {
                reg.SuspendAllReached.Set();
            }
        }

        static void OnBeforeResume(AdditionalStorage &s)
        {
            --reg.SuspendedCount;
            reg.PassedScopedLock[s.ThisTaskIndex].Set();
        }
    };

    using IntTask = Cortado::Task<int, TaskImplWithSuspendedCounter>;

    auto &defaultScheduler =
        Cortado::DefaultScheduler::GetDefaultBackgroundScheduler();

    Cortado::DefaultEvent lockLatch;
    Cortado::DefaultEvent unlockLatch;

    AsyncMutex mutex;

    auto mainTask = [&]() -> Task<>
    {
        using namespace Cortado;
        // Lock mutex from backgound
        //
        co_await defaultScheduler;
        co_await mutex.LockAsync(defaultScheduler);

        // Fire an event that we locked the mutex
        //
        lockLatch.Set();

        // Wait for test body to allow unlocking the mutex
        //
        unlockLatch.Wait();
        mutex.Unlock();
    }();

    ASSERT_TRUE(lockLatch.WaitFor(1000))
        << "mainTask should have locked the mutex";

    // Spawn a task that will lock mutex and wait for an event to proceed later.
    //
    auto task = [&]() -> IntTask
    {
        auto mutexLock = co_await mutex.ScopedLockAsync(defaultScheduler);
        co_return 42;
    };

    IntTask tasks[ConcurrencyCount] =
    {
        task(),
        task(),
        task(),
        task()
    };

    ASSERT_TRUE(reg.SuspendAllReached.WaitFor(2000))
        << "All tasks should reach suspended state!";

    // As we locked mutex before spawning tasks,
    // expect that none of them hold the mutex.
    //
    for (std::size_t i = 0; i < ConcurrencyCount; ++i)
    {
        ASSERT_FALSE(reg.PassedScopedLock[i].IsSet());
    }

    // Unlock the mutex so that `tasks` can start competing for mutex.
    //
    unlockLatch.Set();

    ASSERT_TRUE(mainTask.WaitFor(1000))
        << "Main task should have been completed";

    for (std::size_t i = 0; i < ConcurrencyCount; ++i)
    {
        ASSERT_TRUE(tasks[i].WaitFor(1000))
            << "Task[" << i << "] failed to finish with 1s";
    }

    for (std::size_t i = 0; i < ConcurrencyCount; ++i)
    {
        ASSERT_TRUE(reg.PassedScopedLock[i].IsSet());
    }
}
