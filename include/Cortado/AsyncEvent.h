/// @file AsyncEvent.h
/// Implementation of async mutex.
///

#ifndef CORTADO_ASYNC_EVENT_H
#define CORTADO_ASYNC_EVENT_H

// Cortado
//
#include <Cortado/Concepts/Atomic.h>
#include <Cortado/Concepts/CoroutineScheduler.h>
#include <Cortado/Detail/CoroutineAwaiterQueueNode.h>

// STL
//
#include <limits>

namespace Cortado
{
/// @brief Forward declaration for event awaiter
///
template <Concepts::Atomic AtomicT>
class EventAwaiter;

/// @brief Async event based on atomic vairable
/// @tparam AtomicT Atomic primitive implementation.
///
template <Concepts::Atomic AtomicT>
class AsyncEvent
{
public:
    /// @brief Default constructor.
    ///
    AsyncEvent() = default;

    /// @brief Non-copyable.
    ///
    AsyncEvent(const AsyncEvent &) = delete;

    /// @brief Non-copyable.
    ///
    AsyncEvent &operator=(const AsyncEvent &) = delete;

    /// @brief Destructor.
    //
    ~AsyncEvent() = default;

    /// @brief `co_await`-able event waiter.
    /// @returns Event awaiter.
    ///
    EventAwaiter<AtomicT> WaitAsync();

    /// @brief `co_await`-able event waiter which resumes coroutine on a given
    /// scheduler.
    /// @param sched Scheduler on which to resume the coroutine.
    /// @returns Event awaiter.
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    EventAwaiter<AtomicT> WaitAsync(SchedulerT &sched);

    /// @brief Check if event is already set.
    /// @returns `true` if event is set, false otherwise.
    ///
    bool IsSet() const noexcept
    {
        return m_waitQueue.load(std::memory_order::seq_cst) == EventSet;
    }

    /// @brief Set event and resume awaiting coroutines if any.
    ///
    void Set()
    {
        auto currentState =
            m_waitQueue.exchange(EventSet, std::memory_order::acquire);

        if (currentState == EventNotSet)
        {
            // If we can do futex thing here, then we should do it.
            //
            if constexpr (Concepts::FutexLikeAtomic<AtomicT>)
            {
                m_waitQueue.notify_all();
            }
            return;
        }

        if (currentState == EventSet)
        {
            return;
        }

        // If we can do futex thing here, then we should do it.
        //
        if constexpr (Concepts::FutexLikeAtomic<AtomicT>)
        {
            m_waitQueue.notify_all();
        }
            
        auto *current =
            reinterpret_cast<Detail::CoroutineAwaiterQueueNode *>(currentState);
        for (; current != nullptr; current = current->Next)
        {
            current->Resume();
        }
    }

    /// @brief Enqueue for event wait if event is not set.
    /// @param handleAwaiter Awaiter of coroutine that calls `co_await`.
    /// @returns `true` if awaiter is enqueued, `false` if event was set during
    /// enqueue attempt.
    ///
    bool EnqueueForWait(
        Detail::CoroutineAwaiterQueueNode *handleAwaiter) noexcept
    {
        auto expectedState = m_waitQueue.load(std::memory_order::seq_cst);
        for (;;)
        {
            // If event is set, do not enqueue
            //
            if (expectedState == EventSet)
            {
                return false;
            }
            // Event is not set, try enqueueing.
            //
            else
            {
                // Put LIFO node on top of waiters stack
                //
                handleAwaiter->Next =
                    reinterpret_cast<Detail::CoroutineAwaiterQueueNode *>(
                        expectedState);
                if (m_waitQueue.compare_exchange_weak(
                        expectedState,
                        reinterpret_cast<Concepts::AtomicPrimitive>(
                            handleAwaiter),
                        std::memory_order::acquire,
                        std::memory_order::relaxed))
                {
                    return true;
                }
            }
        }
    }

    /// @brief Sync wait for event.
    /// @tparam AtomicU Is an alias for AtomicT to deduce requirements
    /// only when trying to access Wait method.
    ///
    template <typename AtomicU = AtomicT>
        requires Concepts::FutexLikeAtomic<AtomicU>
    inline void Wait() noexcept
    {
        Concepts::AtomicPrimitive old =
            this->m_waitQueue.load(std::memory_order_acquire);
        while (old != EventSet)
        {
            this->m_waitQueue.wait(old, std::memory_order_acquire);
            old = this->m_waitQueue.load(std::memory_order_acquire);
        }
    }

private:
    static constexpr Concepts::AtomicPrimitive EventNotSet = 0;
    static constexpr Concepts::AtomicPrimitive EventSet =
        (std::numeric_limits<Concepts::AtomicPrimitive>::max)();
    AtomicT m_waitQueue{EventNotSet};
};

/// @brief `co_await`-able event
/// @tparam AtomicT Atomic primitive implementation.
///
template <Concepts::Atomic AtomicT>
class EventAwaiter : public Detail::CoroutineAwaiterQueueNode
{
public:
    /// @brief Constructor.
    /// @param event The AsyncEvent that is to be awaited.
    ///
    EventAwaiter(AsyncEvent<AtomicT> &event) noexcept : m_event{event}
    {
    }

    /// @brief Constructor.
    /// @tparam SchedulerT Type of scheduler on which to resume the coroutine.
    /// @param event The AsyncEvent that is to be awaited.
    /// @param sched The scheduler on which to resume the coroutine.
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    EventAwaiter(AsyncEvent<AtomicT> &event, SchedulerT &scheduler) noexcept :
        m_event{event}
    {
        Detail::CoroutineAwaiterQueueNode::HandleResumerFunc =
            Detail::ScheduleNextWaiter<SchedulerT>;

        Detail::CoroutineAwaiterQueueNode::HandleResumerFuncContext =
            &scheduler;
    }

    /// @brief Compiler contract: Try skipping event wait if it's ready.
    /// @returns true if event is set, false otherwise.
    ///
    bool await_ready() const noexcept
    {
        return this->m_event.IsSet();
    }

    /// @brief Compiler contract: Enqueue coroutine for resumption when it's
    /// set.
    /// @returns true if coroutine enqueued, false if event is ready.
    ///
    template <Concepts::TaskImpl T, typename R>
    bool await_suspend(std::coroutine_handle<PromiseType<T, R>> h) noexcept
    {
        AwaiterBase::await_suspend(h);

        this->HandleToResume = h;

        return this->m_event.EnqueueForWait(this);
    }

    using AwaiterBase::await_resume;

private:
    AsyncEvent<AtomicT> &m_event;
};

/// @brief `co_await`-able event waiter.
/// @returns Event awaiter.
///
template <Concepts::Atomic AtomicT>
EventAwaiter<AtomicT> AsyncEvent<AtomicT>::WaitAsync()
{
    return EventAwaiter<AtomicT>{*this};
}

/// @brief `co_await`-able event waiter which resumes coroutine on a given
/// scheduler.
/// @param sched Scheduler on which to resume the coroutine.
/// @returns Event awaiter.
///
template <Concepts::Atomic AtomicT>
template <Concepts::CoroutineScheduler SchedulerT>
EventAwaiter<AtomicT> AsyncEvent<AtomicT>::WaitAsync(SchedulerT &sched)
{
    return EventAwaiter<AtomicT>{*this, sched};
}
} // namespace Cortado

#endif // CORTADO_ASYNC_EVENT_H
