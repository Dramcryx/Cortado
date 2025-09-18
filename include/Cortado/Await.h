/// @file Await.h
/// Definitions of frequently used awaiters.
///

#ifndef CORTADO_AWAIT_H
#define CORTADO_AWAIT_H

// Cortado
//
#include <Cortado/Concepts/BackgroundResumable.h>
#include <Cortado/Detail/UniquePtrOverArc.h>
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

/// @brief Await for all tasks to complete.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam Args Other tasks.
/// @param first First task to await.
/// @param next Other tasks to await.
/// @return Task<void, T>.
/// 
template <Concepts::TaskImpl T, typename R, typename... Args>
    requires std::is_default_constructible_v<typename T::Allocator>
Task<void, T> WhenAll(Task<R, T> &first, Args &...next)
{
    using AllocatorT = typename T::Allocator;

    return WhenAll(AllocatorT{}, first, next...);
}

/// @brief Await for all tasks to complete.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam Args Other tasks.
/// @param alloc Coroutine allocator.
/// @param first First task to await.
/// @param next Other tasks to await.
/// @return Task<void, T>.
/// 
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

/// @brief Kind of a latch for multiple coroutine competing for coroutine resumption.<br>
/// When child task finished, it tries to steal address from AtomicPrimitive. If it succeeds, it invokes
/// resumption.
/// When caller of WhenAny prepares for suspension, it tries to set address. If it does not succeed,
/// caller gets immediately resumed (i.e. some child has already finished while we were preparing for
/// suspension).
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
///
template <Concepts::TaskImpl T>
struct WhenAnySyncPoint 
{
    /// @brief Awaiter's entry point, tried to set resumption address for child coroutines.
    /// @param h Coroutine handle address.
    /// @returns true if succeeded, false if some child already finished.
    ///
    bool StoreAddress(void* h)
    {
        Concepts::AtomicPrimitive address = reinterpret_cast<Concepts::AtomicPrimitive>(h);
        return m_atomicAddress.compare_exchange_strong(address, 0);
    }

    /// @brief Child tasks' entry point. They try to steal address and replace it with "called" flag.
    ///
    void Invoke()
    {
        Concepts::AtomicPrimitive address = 0;
        if (!m_atomicAddress.compare_exchange_strong(address, WasInvokedValue) && address != WasInvokedValue)
        {
            std::coroutine_handle<>::from_address(reinterpret_cast<void*>(address))();
        }
    }

private:
    static constexpr auto WasInvokedValue = static_cast<Concepts::AtomicPrimitive>(1);
    typename T::Atomic m_atomicAddress{0};
};

/// @brief Awaiter for any of given tasks to complete.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
///
template <Concepts::TaskImpl T>
struct WhenAnyAwaiter : AwaiterBase
{
    Detail::UniquePtrOverArc<WhenAnySyncPoint<T>, typename T::Atomic> SyncPoint;

    /// @brief Compiler contract: Always return false as resumption decision is expected to be made
    /// in await_suspend.
    /// 
    bool await_ready()
    {
        return false;
    }

    /// @brief Compiler contract: Try setting a resumption address in delegate.
    /// @tparam U Return type of child task.
    /// @returns true if address is successfully set, false otherwise.
    ///
    template <typename U>
    bool await_suspend(std::coroutine_handle<PromiseType<T, U>> h)
    {
        AwaiterBase::await_suspend(h);
        return SyncPoint->StoreAddress(h.address());
    }

    /// @brief Compiler contract: Resume action - do nothing, just restore
    /// AwaiterBase state.
    ///
    using AwaiterBase::await_resume;
};

/// @brief Await for any task to complete.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam Args Other tasks.
/// @param alloc Coroutine allocator.
/// @param first First task to await.
/// @param next Other tasks to await.
/// @return WhenAnyAwaiter<T>.
///
template <Concepts::TaskImpl T, typename R, typename... Args>
decltype(auto) WhenAny(typename T::Allocator alloc, Task<R, T> &first, Args &...next)
{
    Detail::UniquePtrOverArc<WhenAnySyncPoint<T>, typename T::Atomic> syncPoint{alloc};

    auto onCompleted = [](Task<R, T>& t, auto sp) -> Task<void, T>
    {
        co_await t;
        sp->Invoke();
    };

    onCompleted(first, syncPoint);
    (onCompleted(next, syncPoint), ...);

    return WhenAnyAwaiter<T>{ .SyncPoint = std::move(syncPoint) };
}

/// @brief Await for any task to complete.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @tparam R Return value type of coroutine that awaits.
/// @tparam Args Other tasks.
/// @param first First task to await.
/// @param next Other tasks to await.
/// @return WhenAnyAwaiter<T>.
///
template <Concepts::TaskImpl T, typename R, typename... Args>
    requires std::is_default_constructible_v<typename T::Allocator>
decltype(auto) WhenAny(Task<R, T> &first, Args &...next)
{
    using AllocatorT = typename T::Allocator;

    return WhenAny(AllocatorT{}, first, next...);
}

} // namespace Cortado

#endif