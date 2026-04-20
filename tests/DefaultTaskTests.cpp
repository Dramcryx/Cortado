/// @file DefaultTaskTests.cpp
/// Tests for Cortado::Task<Cortado::DefaultTaskImpl>.
///

#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/Await.h>

// STL
//
#include <thread>

template <typename T>
using Task = Cortado::Task<T>;

using ThreadIdT = decltype(std::this_thread::get_id());

TEST(DefaultTaskTests, Get_WhenMoveConstructed_Success)
{
    auto task = []() -> Task<int>
    {
        co_await Cortado::ResumeBackground();
        co_return 42;
    };

    auto t1 = task();
    auto t2 = std::move(t1);

    EXPECT_EQ(42, t2.Get());
}

TEST(DefaultTaskTests, Get_WhenMoveAssigned_Success)
{
    auto task = []() -> Task<int>
    {
        co_await Cortado::ResumeBackground();
        co_return 7;
    };

    auto t1 = task();
    auto t2 = task();
    t2 = std::move(t1);

    EXPECT_EQ(7, t2.Get());
}

TEST(DefaultTaskTests, Get_WhenVoidCoroutine_Success)
{
    bool executed = false;
    auto task = [&]() -> Task<void>
    {
        executed = true;
        co_return;
    };

    task().Get();

    EXPECT_TRUE(executed);
}

TEST(DefaultTaskTests, Get_WhenReturnsValue_Success)
{
    auto task = []() -> Task<int>
    {
        co_return 42;
    };

    EXPECT_EQ(42, task().Get());
}

TEST(DefaultTaskTests, Get_WhenThrowsException_Fail)
{
    auto task = []() -> Task<int>
    {
        throw std::runtime_error("From test");
        co_return 42;
    };

    EXPECT_THROW(task().Get(), std::runtime_error);
}

TEST(DefaultTaskTests, Get_WhenResumedOnBackground_Success)
{
    ThreadIdT testThreadId = std::this_thread::get_id();

    auto task = []() -> Task<ThreadIdT>
    {
        co_await Cortado::ResumeBackground();
        co_return std::this_thread::get_id();
    };

    EXPECT_NE(testThreadId, task().Get());
}

TEST(DefaultTaskTests, Get_WhenBackgroundThrows_Fail)
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

TEST(DefaultTaskTests, CoAwait_WhenChildOnSameThread_Success)
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

TEST(DefaultTaskTests, CoAwait_WhenChildOnDifferentThread_Success)
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

TEST(DefaultTaskTests, WhenAll_WhenMultipleTasks_Success)
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

TEST(DefaultTaskTests, CoAwait_WhenCustomScheduler_Success)
{
    using SchedulerT = std::remove_cvref_t<
        decltype(Cortado::DefaultTaskImpl::GetDefaultBackgroundScheduler())>;

    using Cortado::operator co_await;

    auto testThreadId = std::this_thread::get_id();
    auto backgroundThreadId = testThreadId;

    SchedulerT sched;
    static auto task1 = [&]() -> Task<void>
    {
        co_await sched;

        backgroundThreadId = std::this_thread::get_id();
    };

    task1().Get();

    EXPECT_NE(testThreadId, backgroundThreadId);
}

TEST(DefaultTaskTests, WaitFor_WhenSlowTask_Success)
{
    static auto task1 = [&]() -> Task<void>
    {
        co_await Cortado::ResumeBackground();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    };

    auto taskObject = task1();

    EXPECT_FALSE(taskObject.WaitFor(10));
    EXPECT_TRUE(taskObject.WaitFor(1000));
}

TEST(DefaultTaskTests, WhenAny_WhenMultipleTasks_Success)
{
    static std::atomic_int v{0};
    static auto task1 = []() -> Task<int>
    {
        co_await Cortado::ResumeBackground();
        co_return ++v;
    };

    []() -> Task<void>
    {
        Task<int> tasks[] = {task1(), task1(), task1()};
        co_await Cortado::WhenAny(tasks[0], tasks[1], tasks[2]);
    }()
                .Get();

    EXPECT_GE(v.load(), 1) << "At least one task must have completed";
}

TEST(DefaultTaskTests, WhenAny_WhenTaskThrows_Success)
{
    auto failingTask = []() -> Task<int>
    {
        co_await Cortado::ResumeBackground();
        throw std::runtime_error("WhenAny failure");
        co_return 0;
    };

    auto slowTask = []() -> Task<int>
    {
        co_await Cortado::ResumeBackground();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        co_return 1;
    };

    auto wrapper = [&]() -> Task<void>
    {
        Task<int> tasks[] = {failingTask(), slowTask()};
        co_await Cortado::WhenAny(tasks[0], tasks[1]);
    };

    auto t = wrapper();
    EXPECT_TRUE(t.WaitFor(1000))
        << "WhenAny must complete promptly even if a task throws";
}
