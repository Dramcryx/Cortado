#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/Await.h>

// STL
//
#include <stdexcept>
#include <thread>
#include <type_traits>

template <typename T>
using Task = Cortado::Task<T>;

using ThreadIdT = decltype(std::this_thread::get_id());

TEST(DefaultTaskTests, CompletedFromValue)
{
    auto task = []() -> Task<int>
    {
        co_return 42;
    };

    EXPECT_EQ(42, task().Get());
}

TEST(DefaultTaskTests, CompletedFromStdException)
{
    auto task = []() -> Task<int>
    {
        throw std::runtime_error("From test");
        co_return 42;
    };

    EXPECT_THROW(task().Get(), std::runtime_error);
}

TEST(DefaultTaskTests, CompletedInBackgroundThread)
{
    ThreadIdT testThreadId = std::this_thread::get_id();

    auto task = []() -> Task<ThreadIdT>
    {
        co_await Cortado::ResumeBackground();
        co_return std::this_thread::get_id();
    };

    EXPECT_NE(testThreadId, task().Get());
}

TEST(DefaultTaskTests, RethrowFromBackgroundThread)
{
    ThreadIdT testThreadId = std::this_thread::get_id();

    auto task = [](ThreadIdT &out) -> Task<void>
    {
        co_await Cortado::ResumeBackground();
        out = std::this_thread::get_id();

        throw std::runtime_error("From test");
    };

    ThreadIdT backgoundThreadId;
    auto taskObject = task(backgoundThreadId);

    EXPECT_THROW(taskObject.Get(), std::runtime_error);
    EXPECT_NE(testThreadId, backgoundThreadId);
}

TEST(DefaultTaskTests, AwaitForOtherTaskOnSameThread)
{
    static constexpr int firstTaskValue = 32;
    static constexpr int secondTaskAdds = 1;
    static auto task1 = []() -> Task<int>
    {
        co_return firstTaskValue;
    };

    static auto task2 = []() -> Task<int>
    {
        co_return (co_await task1()) + secondTaskAdds;
    };

    EXPECT_EQ(firstTaskValue + secondTaskAdds, task2().Get());
}

TEST(DefaultTaskTests, AwaitForOtherTaskOnDifferentThreads)
{
    static constexpr int firstTaskValue = 32;
    static constexpr int secondTaskAdds = 1;

    ThreadIdT task1ThreadId;

    static auto task1 = [](ThreadIdT &out) -> Task<int>
    {
        co_await Cortado::ResumeBackground();

        out = std::this_thread::get_id();

        co_return firstTaskValue;
    };

    static auto task2 = [](ThreadIdT &out) -> Task<int>
    {
        co_return (co_await task1(out)) + secondTaskAdds;
    };

    EXPECT_EQ(firstTaskValue + secondTaskAdds, task2(task1ThreadId).Get());
    EXPECT_NE(std::this_thread::get_id(), task1ThreadId);
}

TEST(DefaultTaskTests, WhenAll)
{
    static auto task1 = []() -> Task<int>
    {
        co_await Cortado::ResumeBackground();

        co_return rand();
    };

    Task<int> tasks[] = {task1(), task1(), task1()};

    Cortado::WhenAll(tasks[0], tasks[1], tasks[2]).Get();

    EXPECT_TRUE(tasks[0].IsReady());
    EXPECT_TRUE(tasks[1].IsReady());
    EXPECT_TRUE(tasks[2].IsReady());
}

TEST(DefaultTaskTests, AwaitOnScheduler)
{
    using SchedulerT = std::remove_cvref_t<
        decltype(Cortado::DefaultTaskImpl::GetDefaultBackgroundScheduler())>;

    using Cortado::operator co_await;

    auto testThreadId = std::this_thread::get_id();
    auto backgroundThreadId = testThreadId;

    static auto task1 = [&]() -> Task<void>
    {
        SchedulerT sched;
        co_await sched;

        backgroundThreadId = std::this_thread::get_id();
    };

    task1().Get();

    EXPECT_NE(testThreadId, backgroundThreadId);
}
