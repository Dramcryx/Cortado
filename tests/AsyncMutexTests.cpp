/// @file AsyncMutexTests.cpp
/// Tests for Cortado::AsyncMutex
///

#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/AsyncMutex.h>
#include <Cortado/Await.h>

template <typename T = void>
using Task = Cortado::Task<T>;

using AsyncMutex = Cortado::AsyncMutex<std::atomic_int64_t>;

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

    // Call task and expect it to suspend because we hold the lock from this
    // thread
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

    auto mainTaskLambda = [&]() -> Task<>
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
    };

    auto mainTask = mainTaskLambda();

    ASSERT_TRUE(lockLatch.WaitFor(1000))
        << "mainTask should have locked the mutex";

    // Spawn a task that will lock mutex and wait for an event to proceed later.
    //
    auto task = [&]() -> IntTask
    {
        auto mutexLock = co_await mutex.ScopedLockAsync(defaultScheduler);
        co_return 42;
    };

    IntTask tasks[ConcurrencyCount] = {task(), task(), task(), task()};

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

TEST(AsyncMutexTests, TryLockWhileLocked)
{
    AsyncMutex m;

    // Lock manually
    //
    ASSERT_TRUE(m.TryLock());

    // Lock manually again, should not succeed
    //
    EXPECT_FALSE(m.TryLock());

    // Release mutex
    //
    m.Unlock();
}

TEST(AsyncMutexTests, ScopedLockUnlocksAfterException)
{
    AsyncMutex m;

    ASSERT_TRUE(m.TryLock());

    auto task = [&]() -> Cortado::Task<void>
    {
        auto lock = co_await m.ScopedLockAsync(
            Cortado::DefaultScheduler::GetDefaultBackgroundScheduler());
        throw std::runtime_error("FromLambda");
    };

    m.Unlock();

    EXPECT_ANY_THROW(task().Get());

    EXPECT_TRUE(m.TryLock());
}

TEST(AsyncMutexAdditionalTests, StressOnDefaultScheduler)
{
    constexpr std::size_t Threads = 8;
    constexpr std::size_t Iterations = 2000;

    AsyncMutex m;
    std::atomic<std::int64_t> counter{0};

    auto &sched = Cortado::DefaultScheduler::GetDefaultBackgroundScheduler();
    auto worker = [&]() -> Cortado::Task<void>
    {
        for (std::size_t i = 0; i < Iterations; ++i)
        {
            auto guard = co_await m.ScopedLockAsync(sched);
            counter.fetch_add(1, std::memory_order_relaxed);
        }
    };

    Cortado::Task<void> tasks[Threads]{worker(),
                                       worker(),
                                       worker(),
                                       worker(),
                                       worker(),
                                       worker(),
                                       worker(),
                                       worker()};

    auto waitAllTask = Cortado::WhenAll(tasks[0],
                                        tasks[1],
                                        tasks[2],
                                        tasks[3],
                                        tasks[4],
                                        tasks[5],
                                        tasks[6],
                                        tasks[7]);

    EXPECT_TRUE(waitAllTask.WaitFor(2000));

    EXPECT_TRUE(m.TryLock());
    EXPECT_EQ(counter.load(), Threads * Iterations);
}
