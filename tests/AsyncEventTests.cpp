/// @file AsyncEventTests.cpp
/// Tests for Cortado::AsyncEvent
///

#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/AsyncEvent.h>
#include <Cortado/Await.h>

using AsyncEvent = Cortado::AsyncEvent<Cortado::Common::STLAtomic::Atomic>;
using DefaultScheduler = Cortado::DefaultScheduler;

template <typename R>
using Task = Cortado::Task<R>;

TEST(AsyncEventTests, BasicWaitSet)
{
    // Basic await and set
    //

    AsyncEvent ev; // event starts in “not set” state

    // 1) co_await should suspend immediately
    auto task = [&]() -> Task<int>
    {
        co_await ev.WaitAsync(); // awaiter is created
        co_return 42;            // will be returned after Set()
    };

    auto t = task(); // coroutine starts, immediately suspends

    EXPECT_FALSE(t.IsReady()); // not finished yet

    // 2) Setting the event should resume the coroutine
    ev.Set();

    EXPECT_TRUE(t.IsReady()); // finished now
    EXPECT_EQ(t.Get(), 42);   // value propagated
}

TEST(AsyncEventTests, WaitSetTwice)
{
    // Event is awaited by two tasks
    //

    AsyncEvent ev;

    // First waiter
    auto t1 = [&]() -> Task<int>
    {
        co_await ev.WaitAsync();
        co_return 1;
    }();

    // Second waiter
    auto t2 = [&]() -> Task<int>
    {
        co_await ev.WaitAsync();
        co_return 2;
    }();

    // No waiter should be ready yet
    EXPECT_FALSE(t1.IsReady());
    EXPECT_FALSE(t2.IsReady());

    // Set the event once – both waiters should be resumed
    ev.Set();

    EXPECT_TRUE(t1.IsReady());
    EXPECT_TRUE(t2.IsReady());
    EXPECT_EQ(t1.Get(), 1);
    EXPECT_EQ(t2.Get(), 2);
}

TEST(AsyncEventTests, WaitWithScheduler)
{
    // A coroutine awaits an event, passing scheduler
    // on which it is going to resume once event is set.
    //

    AsyncEvent ev;
    DefaultScheduler &sched = DefaultScheduler::
        GetDefaultBackgroundScheduler(); // a simple single‑thread scheduler

    // Coroutine that will run on `sched`
    auto t = [&]() -> Task<int>
    {
        co_await ev.WaitAsync(sched); // awaiter stores the scheduler
        co_return 99;
    }();

    EXPECT_FALSE(t.IsReady()); // suspended

    ev.Set(); // event set, coroutine should be queued on `sched`

    EXPECT_TRUE(t.WaitFor(1000));
    EXPECT_EQ(t.Get(), 99);
}

TEST(AsyncEventTests, MultipleWaiters)
{
    // Create five awaiters
    //

    constexpr std::size_t N = 5;
    AsyncEvent ev;

    auto worker = [&]() -> Task<void>
    {
        co_await ev.WaitAsync();
    };

    // All tasks wait on the same event
    Task<void> tasks[N]{worker(), worker(), worker(), worker(), worker()};

    // None should be ready before Set()
    for (std::size_t i = 0; i < N; ++i)
    {
        EXPECT_FALSE(tasks[i].IsReady());
    }

    // Set the event – all waiters should resume
    ev.Set();

    for (std::size_t i = 0; i < N; ++i)
    {
        EXPECT_TRUE(tasks[i].IsReady());
    }
}

TEST(AsyncEventTests, AwaiterException)
{
    // Test coroutine error completion after event is signaled.
    //

    AsyncEvent ev;

    auto t = [&]() -> Task<void>
    {
        co_await ev.WaitAsync(); // wait
        throw std::runtime_error("boom");
    }();

    // Coroutine is suspended – event not set yet
    EXPECT_FALSE(t.IsReady());

    // Set the event – coroutine resumes and throws
    ev.Set();

    EXPECT_ANY_THROW(t.Get()); // Get() rethrows the exception

    // After a thrown coroutine, subsequent waiters should still work
    auto t2 = [&]() -> Task<int>
    {
        co_await ev.WaitAsync();
        co_return 123;
    }();

    // Since event is already set, waiter should not suspend
    EXPECT_TRUE(t2.IsReady());
    EXPECT_EQ(t2.Get(), 123);
}

TEST(AsyncEventTests, LatchLikeBehavior)
{
    // Event is set only once; after that all waiters are released
    AsyncEvent latch;

    // Waiter 1
    auto t1 = [&]() -> Task<void>
    {
        co_await latch.WaitAsync();
    }();

    // Waiter 2
    auto t2 = [&]() -> Task<void>
    {
        co_await latch.WaitAsync();
    }();

    EXPECT_FALSE(t1.IsReady());
    EXPECT_FALSE(t2.IsReady());

    // Set the event – both should resume
    latch.Set();

    EXPECT_TRUE(t1.IsReady());
    EXPECT_TRUE(t2.IsReady());

    // Subsequent waiters should resume immediately
    auto t3 = [&]() -> Task<void>
    {
        co_await latch.WaitAsync();
    }();

    EXPECT_TRUE(t3.IsReady()); // no suspension
}

TEST(AsyncEventTests, SyncWait)
{
    AsyncEvent event;

    auto fireEventTaskLambda = [&]() -> Task<void>
    {
        co_await Cortado::ResumeBackground();
        event.Set();
    };

    ASSERT_FALSE(event.IsSet());

    auto fireEventTask = fireEventTaskLambda();

    event.Wait();

    ASSERT_TRUE(event.IsSet());
    ASSERT_TRUE(fireEventTask.WaitFor(2000));
}
