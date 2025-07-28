#ifndef CORTADO_AWAIT_H
#define CORTADO_AWAIT_H

// Cortado
//
#include "Cortado/Concepts/CoroutineScheduler.h"
#include <Cortado/Concepts/BackgroundResumable.h>
#include <Cortado/Task.h>

namespace Cortado
{

namespace Detail
{
using BeforeResumeFuncT = void (*)(std::coroutine_handle<>);

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

struct AwaiterBase
{
protected:
    using Base = AwaiterBase;

    template <Concepts::TaskImpl T, typename R>
    inline void await_suspend(
        std::coroutine_handle<Cortado::PromiseType<T, R>> h)
    {
        m_handle = h;
        m_beforeResumeFunc = Detail::BeforeResumeFunc<T, R>;
        h.promise().BeforeSuspend();
    }

    inline void await_resume()
    {
        if (m_handle)
        {
            m_beforeResumeFunc(m_handle);
        }
    }

private:
    std::coroutine_handle<> m_handle;
    Detail::BeforeResumeFuncT m_beforeResumeFunc;
};

template <typename R, Concepts::TaskImpl T>
struct Task<R, T>::TaskAwaiter : AwaiterBase
{
    TaskAwaiter(Task<R, T> &&task) :
        m_awaitedTask(std::forward<Task<R, T>>(task))
    {
    }

    bool await_ready()
    {
        return m_awaitedTask.m_handle.promise().Ready();
    }

    template <Concepts::TaskImpl T2, typename R2>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<T2, R2>> h)
    {
        Base::await_suspend(h);
        m_awaitedTask.m_handle.promise().SetContinuation(h);
    }

    R await_resume()
    {
        return m_awaitedTask.Get();
    }

private:
    Task<R, T> m_awaitedTask;
};

template <typename R, Concepts::TaskImpl T>
auto operator co_await(Task<R, T> &&rvalue)
{
    return typename Task<R, T>::TaskAwaiter{std::forward<Task<R, T>>(rvalue)};
}

template <typename R, Concepts::TaskImpl T>
struct Task<R, T>::TaskLValueAwaier : AwaiterBase
{
    TaskLValueAwaier(Task<R, T> &awaitedTask) : m_awaitedTask{awaitedTask}
    {
    }

    bool await_ready()
    {
        return m_awaitedTask.m_handle.promise().Ready();
    }

    template <Concepts::TaskImpl T2, typename R2>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<T2, R2>> h)
    {
        Base::await_suspend(h);
        m_awaitedTask.m_handle.promise().SetContinuation(h);
    }

    using Base::await_resume;

private:
    Task<R, T> &m_awaitedTask;
};

template <typename R, Concepts::TaskImpl T>
auto operator co_await(Task<R, T> &lvalue)
{
    return typename Task<R, T>::TaskLValueAwaier{lvalue};
}

struct ResumeBackgroundAwaiter : AwaiterBase
{
    bool await_ready()
    {
        return false;
    }

    template <Concepts::BackgroundResumable T, typename R>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<T, R>> h)
    {
        Base::await_suspend(h);

        T::GetDefaultBackgroundScheduler().Schedule(h);
    }

    using Base::await_resume;

private:
    std::coroutine_handle<> m_handle;
    Detail::BeforeResumeFuncT m_beforeResumeFunc;
};

inline ResumeBackgroundAwaiter ResumeBackground()
{
    return {};
}

template <Concepts::TaskImpl T, typename R, typename... Args>
Task<void, T> WhenAll(Task<R, T> &first, Args &&...next)
{
    co_await first;
    (void(co_await next), ...);
}

template <Concepts::CoroutineScheduler T>
struct CoroutineSchedulerAwaiter : AwaiterBase
{
    CoroutineSchedulerAwaiter(T &sched) : m_scheduler{sched}
    {
    }

    bool await_ready()
    {
        return false;
    }

    template <Concepts::TaskImpl TTask, typename R>
    void await_suspend(std::coroutine_handle<Cortado::PromiseType<TTask, R>> h)
    {
        Base::await_suspend(h);

        m_scheduler.Schedule(h);
    }

    using AwaiterBase::await_resume;

private:
    T &m_scheduler;
};

template <Concepts::CoroutineScheduler T>
auto operator co_await(T &sched)
{
    return CoroutineSchedulerAwaiter{sched};
};

} // namespace Cortado

#endif