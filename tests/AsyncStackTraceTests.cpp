/// @file AsyncStackTraceTests.cpp
/// Tests for async stack tracing.
///

#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/DefaultAsyncStackTLS.h>
#include <Cortado/DefaultTaskImpl.h>
#include <Cortado/Detail/AsyncStackFrame.h>

// STL
//
#include <vector>

namespace
{

using TLS = Cortado::DefaultAsyncStackTLS;
using AsyncFrame = Cortado::Detail::AsyncStackFrame<TLS>;


struct TaskImplWithAsyncStack :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    Cortado::DefaultScheduler
{
    using Event = Cortado::DefaultEvent;
    using AsyncStackTLSProvider = TLS;
};

static_assert(
    Cortado::Concepts::AsyncStackTracing<TaskImplWithAsyncStack>,
    "TaskImplWithAsyncStack should satisfy AsyncStackTracing concept");

template <typename T = void>
using TracedTask = Cortado::Task<T, TaskImplWithAsyncStack>;

int GetAsyncStackDepth()
{
    int depth = 0;
    Cortado::Detail::WalkCurrentAsyncStack<TLS>(
        [&](const AsyncFrame &) -> bool
        {
            ++depth;
            return true;
        });
    return depth;
}

std::vector<const AsyncFrame *> GetAsyncStackFrames()
{
    std::vector<const AsyncFrame *> frames;
    Cortado::Detail::WalkCurrentAsyncStack<TLS>(
        [&](const AsyncFrame &frame) -> bool
        {
            frames.push_back(&frame);
            return true;
        });
    return frames;
}

TracedTask<int> LeafCoroutine(int *depthOut)
{
    *depthOut = GetAsyncStackDepth();
    co_return 1;
}

TracedTask<int> MiddleCoroutine(int *depthOut)
{
    auto result = co_await LeafCoroutine(depthOut);
    co_return result;
}

TracedTask<int> RootCoroutine(int *depthOut)
{
    auto result = co_await MiddleCoroutine(depthOut);
    co_return result;
}

} // namespace

TEST(AsyncStackTraceTests, WalkStack_WhenNestedCoroutines_CorrectDepth)
{
    int depth = 0;
    auto task = RootCoroutine(&depth);
    task.Get();

    EXPECT_EQ(3, depth) << "Expected 3 frames: Root -> Middle -> Leaf";
}

TEST(AsyncStackTraceTests, WalkStack_WhenParentLinks_CorrectChain)
{
    bool chainValid = false;

    auto leaf = [&]() -> TracedTask<void>
    {
        auto frames = GetAsyncStackFrames();
        // Validate chain while frames are still alive
        if (frames.size() == 3)
        {
            chainValid =
                frames[0]->parentFrame == frames[1] &&
                frames[1]->parentFrame == frames[2] &&
                frames[2]->parentFrame == nullptr;
        }
        co_return;
    };

    auto middle = [&]() -> TracedTask<void>
    {
        co_await leaf();
    };

    auto root = [&]() -> TracedTask<void>
    {
        co_await middle();
    };

    root().Get();

    EXPECT_TRUE(chainValid)
        << "Each frame's parentFrame should point to the next in the chain";
}

TEST(AsyncStackTraceTests, WalkStack_WhenCoroutineCompletes_StackEmpty)
{
    auto task = []() -> TracedTask<int>
    {
        co_return 42;
    };

    task().Get();

    EXPECT_EQ(nullptr, AsyncFrame::GetCurrent())
        << "Stack should be empty after all coroutines complete";
}

TEST(AsyncStackTraceTests, WalkStack_WhenSuspendAndResume_StackRestored)
{
    int depthBeforeSuspend = 0;

    static auto inner = [](int *_depthBeforeSuspend) -> TracedTask<int>
    {
        *_depthBeforeSuspend = GetAsyncStackDepth();
        co_await Cortado::ResumeBackground();
        co_return GetAsyncStackDepth();
    };

    auto outer = [](int *_depthBeforeSuspend) -> TracedTask<int>
    {
        co_return co_await inner(_depthBeforeSuspend);
    };

    int depthAfterResume = outer(&depthBeforeSuspend).Get();

    EXPECT_EQ(2, depthBeforeSuspend)
        << "Should see 2 frames before suspend";
    EXPECT_EQ(2, depthAfterResume)
        << "Should see 2 frames after resume on background thread";
}

TEST(AsyncStackTraceTests, WalkStack_WhenDefaultTaskImpl_NoCost)
{
    // Ensure DefaultTaskImpl (without AsyncStackTLSProvider) doesn't
    // interfere with the TLS.
    AsyncFrame::SetCurrent(nullptr);

    auto task = []() -> Cortado::Task<int>
    {
        co_return 99;
    };

    EXPECT_EQ(99, task().Get());
    EXPECT_EQ(nullptr, AsyncFrame::GetCurrent())
        << "DefaultTaskImpl should not touch async stack TLS";
}
