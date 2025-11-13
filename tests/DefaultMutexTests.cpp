/// @file DefaultMutexTests.cpp
/// Tests for Cortado::DefaultMutex.
///

#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/DefaultMutex.h>

TEST(DefaultMutexTests, BasicLock)
{
    Cortado::DefaultMutex mutex;

    mutex.lock();

    ASSERT_FALSE(mutex.try_lock()) << "Repetitive lock must fail";

    mutex.unlock();

    ASSERT_TRUE(mutex.try_lock()) << "Lock after unlock must succeed";
}

TEST(DefaultMutexTests, BasicConcurrency)
{
    Cortado::DefaultMutex mutex;

    int value = 1;

    mutex.lock();

    ASSERT_FALSE(mutex.try_lock());

    auto backgroundTask = [&]() -> Cortado::Task<void>
    {
        co_await Cortado::ResumeBackground();
        std::lock_guard lk{mutex};
        value /= 2;
    }();

    value *= 2;

    mutex.unlock();

    ASSERT_TRUE(backgroundTask.WaitFor(10000))
        << "Background task must finish";

    ASSERT_EQ(1, value);
}

TEST(DefaultMutexTests, StrongerConcurrency)
{
    Cortado::DefaultMutex mutex;

    int value = 1;

    mutex.lock();

    auto multiplierTask = [&]() -> Cortado::Task<void>
    {
        co_await Cortado::ResumeBackground();
        std::lock_guard lk{mutex};
        value *= 2;
    };

    auto dividerTask = [&]() -> Cortado::Task<void>
    {
        co_await Cortado::ResumeBackground();
        std::lock_guard lk{mutex};
        value /= 2;
    };

    Cortado::Task<void> tasks[]{multiplierTask(),
                                dividerTask(),
                                multiplierTask(),
                                dividerTask(),
                                multiplierTask(),
                                dividerTask()};
    
    mutex.unlock();

    auto whenAllTask = Cortado::WhenAll(tasks[0],
                                        tasks[1],
                                        tasks[2],
                                        tasks[3],
                                        tasks[4],
                                        tasks[5]);

    ASSERT_TRUE(whenAllTask.WaitFor(1000)) << "Background tasks must finish";

    ASSERT_EQ(1, value);
}
