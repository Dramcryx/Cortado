/// @file Await.h
/// Definitions of frequently used awaiters.
///

#ifndef CORTADO_AWAIT_H
#define CORTADO_AWAIT_H

// Cortado
//
#include <Cortado/Concepts/BackgroundResumable.h>
#include <Cortado/Task.h>

namespace Cortado
{

namespace Detail
{
/// @brief Type-eraser for awaiters to call before resumption.
///
using BeforeResumeFuncT = void (*)(std::coroutine_handle<>);

/// @brief Type-erased function for awairers to call before resumption.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @tparam R Return type of coroutine.
///
template <Concepts::TaskImpl T, typename R>
void BeforeResumeFunc(std::coroutine_handle<> h)
{
    if constexpr (Concepts::HasAdditionalStorage<T>)
    {
        auto restored =
            std::coroutine_handle<Cortado::PromiseType<T, R>>::from_address(
                h.address());
        restored.promise().BeforeResume();
    }
}
} // namespace Detail

/// @brief Core awaiter struct. Reusable code for user storage.
///
struct AwaiterBase
{
protected:
    using Base = AwaiterBase;

    /// @brief Actions before suspend. If no user storage is defined,
    /// this method is no-op.
    ///
    template <Concepts::TaskImpl T, typename R>
    inline void await_suspend(
        std::coroutine_handle<Cortado::PromiseType<T, R>> h)
    {
        if constexpr (Concepts::HasAdditionalStorage<T>)
        {
            m_handle = h;
            m_beforeResumeFunc = Detail::BeforeResumeFunc<T, R>;
            h.promise().BeforeSuspend();
        }
    }

    /// @brief Actions before resumption. If no user storage is defined,
    /// this method is no-op.
    ///
    inline void await_resume()
    {
        if (m_handle)
        {
            m_beforeResumeFunc(m_handle);
        }
    }

private:
    std::coroutine_handle<> m_handle{nullptr};
    Detail::BeforeResumeFuncT m_beforeResumeFunc;
};

/// @brief Task-to-task awaiter implementation.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
///
template <typename R, Concepts::TaskImpl T>
struct Task<R, T>::TaskAwaiter : AwaiterBase
{
    /// @brief Constructor. Saving a task that we are going to resume.
    /// @param task The task to resume.
    ///
    TaskAwaiter(Task<R, T> &&task) :
        m_awaitedTask(std::forward<Task<R, T>>(task))
    {
    }

    /// @brief Compiler contract: If task is ready, we can immediately resume.
    ///
    bool await_ready()
    {
        return m_awaitedTask.m_handle.promise().Ready();
    }

    /// @brief Compiler contract: Suspend actions if co_await target is not
    /// ready.
    ///
    template <Concepts::TaskImpl T2, typename R2>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<T2, R2>> h)
    {
        Base::await_suspend(h);
        m_awaitedTask.m_handle.promise().SetContinuation(h);
    }

    /// @brief Compiler contract: Resume action - take co_await's target result.
    ///
    R await_resume()
    {
        return m_awaitedTask.Get();
    }

private:
    Task<R, T> m_awaitedTask;
};

/// @brief Compiler contract: co_await operator implementation when a task
/// awaits for another task.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @returns TaskAwaiter.
///
template <typename R, Concepts::TaskImpl T>
auto operator co_await(Task<R, T> &&rvalue)
{
    return typename Task<R, T>::TaskAwaiter{std::forward<Task<R, T>>(rvalue)};
}

/// @brief Task-to-task awaiter implementation which only awaits completion
/// without taking result.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
///
template <typename R, Concepts::TaskImpl T>
struct Task<R, T>::TaskLValueAwaier : AwaiterBase
{
    /// @brief Constructor. Saving a task that we are going to resume.
    /// @param task The task to resume.
    ///
    TaskLValueAwaier(Task<R, T> &awaitedTask) : m_awaitedTask{awaitedTask}
    {
    }

    /// @brief Compiler contract: If task is ready, we can immediately resume.
    ///
    bool await_ready()
    {
        return m_awaitedTask.m_handle.promise().Ready();
    }

    /// @brief Compiler contract: Suspend actions if co_await target is not
    /// ready.
    ///
    template <Concepts::TaskImpl T2, typename R2>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<T2, R2>> h)
    {
        Base::await_suspend(h);
        m_awaitedTask.m_handle.promise().SetContinuation(h);
    }

    /// @brief Compiler contract: Resume action - do nothing, just restore
    /// AwaiterBase state.
    ///
    using Base::await_resume;

private:
    Task<R, T> &m_awaitedTask;
};

/// @brief Compiler contract: co_await operator implementation when a task
/// awaits for another task.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @returns TaskLValueAwaier.
///
template <typename R, Concepts::TaskImpl T>
auto operator co_await(Task<R, T> &lvalue)
{
    return typename Task<R, T>::TaskLValueAwaier{lvalue};
}

/// @brief Awaiter that transfers coroutine execution to a default scheduler.
///
struct ResumeBackgroundAwaiter : AwaiterBase
{
    /// @brief Compiler contract: We indicate that a task is not ready to
    /// always transfer task to the scheduler.
    ///
    bool await_ready()
    {
        return false;
    }

    /// @brief Compiler contract: Suspend actions - suspend and move to a the
    /// scheduler.
    ///
    template <Concepts::BackgroundResumable T, typename R>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<T, R>> h)
    {
        Base::await_suspend(h);

        T::GetDefaultBackgroundScheduler().Schedule(h);
    }

    /// @brief Compiler contract: Resume action - do nothing, just restore
    /// AwaiterBase state.
    ///
    using Base::await_resume;

private:
    std::coroutine_handle<> m_handle;
    Detail::BeforeResumeFuncT m_beforeResumeFunc;
};

/// @brief co_await shortcut for resuming on a different thread.
///
inline ResumeBackgroundAwaiter ResumeBackground()
{
    return {};
}

template <Concepts::TaskImpl T, typename R, typename... Args>
    requires std::is_default_constructible_v<typename T::Allocator>
Task<void, T> WhenAll(Task<R, T> &first, Args &...next)
{
    using AllocatorT = typename T::Allocator;

    return WhenAll(AllocatorT{}, first, next...);
}

template <Concepts::TaskImpl T, typename R, typename... Args>
Task<void, T> WhenAll(typename T::Allocator alloc,
                      Task<R, T> &first,
                      Args &...next)
{
    co_await first;
    (void(co_await next), ...);
}

/// @brief Awaiter that transfers coroutine execution to a specified scheduler.
///
template <Concepts::CoroutineScheduler T>
struct CoroutineSchedulerAwaiter : AwaiterBase
{
    /// @brief Constructor. Saving a scheduler on which we will resume
    /// coroutine.
    /// @param sched The target scheduler.
    ///
    CoroutineSchedulerAwaiter(T &sched) : m_scheduler{sched}
    {
    }

    /// @brief Compiler contract: We indicate that a task is not ready to
    /// always transfer task to the scheduler.
    ///
    bool await_ready()
    {
        return false;
    }

    /// @brief Compiler contract: Suspend actions - suspend and move to a the
    /// scheduler.
    ///
    template <Concepts::TaskImpl TTask, typename R>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<TTask, R>> h)
    {
        Base::await_suspend(h);

        m_scheduler.Schedule(h);
    }

    /// @brief Compiler contract: Resume action - do nothing, just restore
    /// AwaiterBase state.
    ///
    using AwaiterBase::await_resume;

private:
    T &m_scheduler;
};

/// @brief co_await implementation to transfer on specific scheduler.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @param sched Scheduler to which coroutine is transfered.
/// @returns CoroutineSchedulerAwaiter.
///
template <Concepts::CoroutineScheduler T>
auto operator co_await(T &sched)
{
    return CoroutineSchedulerAwaiter{sched};
};

} // namespace Cortado

#endif