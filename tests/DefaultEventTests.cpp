/// @file DefaultEventTests.cpp
/// Tests for Cortado::DefaultEvent.
///

#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/DefaultEvent.h>

TEST(DefaultEventTests, BasicSet)
{
    Cortado::DefaultEvent event;

    ASSERT_FALSE(event.IsSet());

    ASSERT_FALSE(event.WaitFor(100)) << "Event must not be fired";

    event.Set();

    ASSERT_TRUE(event.IsSet()) << "Event was set!";

    ASSERT_TRUE(event.WaitFor(100)) << "Event was set, no need to wait!";
}

TEST(DefaultEventTests, BasicConcurrency)
{
    Cortado::DefaultEvent event;

    int value = 1;

    auto backgroundTask = [&]() -> Cortado::Task<void>
    {
        co_await Cortado::ResumeBackground();
        event.WaitFor(5000);
        value /= 2;
    }();

    value *= 2;

    event.Set();

    ASSERT_TRUE(backgroundTask.WaitFor(1000)) << "Background task must finish";

    ASSERT_EQ(1, value);
}

TEST(DefaultEventTests, StrongerConcurrency)
{
    Cortado::DefaultEvent event;

    std::atomic_int64_t value = 1;

    auto incrementTask = [&]() -> Cortado::Task<void>
    {
        co_await Cortado::ResumeBackground();
        event.WaitFor(5000);
        ++value;
    };

    auto decrementTask = [&]() -> Cortado::Task<void>
    {
        co_await Cortado::ResumeBackground();
        event.WaitFor(5000);
        --value;
    };

    Cortado::Task<void> tasks[]{incrementTask(),
                                decrementTask(),
                                incrementTask(),
                                decrementTask(),
                                incrementTask(),
                                decrementTask()};

    event.Set();

    auto whenAllTask = Cortado::WhenAll(tasks[0],
                                        tasks[1],
                                        tasks[2],
                                        tasks[3],
                                        tasks[4],
                                        tasks[5]);

    ASSERT_TRUE(whenAllTask.WaitFor(1000)) << "Background tasks must finish";

    ASSERT_EQ(1, value);
}
