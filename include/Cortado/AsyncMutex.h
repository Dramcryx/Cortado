/// @file AsyncMutex.h
/// Implementation of async mutex.
///

#ifndef CORTADO_ASYNC_MUTEX_H
#define CORTADO_ASYNC_MUTEX_H

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Concepts/CoroutineScheduler.h>

// STL
//
#include <atomic>
#include <limits>

namespace Cortado
{
namespace Detail
{
/// @brief Base for all of the mutex awaiters, for storing in a linked-list.
///
struct LockAwaiterNode : AwaiterBase
{
    std::coroutine_handle<> HandleToResume{nullptr};
    void (*HandleResumerFunc)(std::coroutine_handle<>, void *) = nullptr;
    void *HandleResumerFuncContext = nullptr;
    LockAwaiterNode *Next{nullptr};

    inline void Resume()
    {
        if (HandleToResume == nullptr)
        {
            return;
        }

        // Resume waiter right here if there is no scheduler.
        //
        if (HandleResumerFunc == nullptr)
        {
            HandleToResume.resume();
            return;
        }

        // If we have a handler, we are likely asked to schedule
        // next waiter on a scheduler.
        //
        HandleResumerFunc(HandleToResume, HandleResumerFuncContext);
    }
};

/// @brief Common function for both awaiters to resume
/// next awaiting coroutine asymmetrically.
/// @param h Coroutine which takes the lock next.
/// @param context Type-erased scheduler.
///
template <Concepts::CoroutineScheduler SchedulerT>
inline void ScheduleNextWaiter(std::coroutine_handle<> h, void *context)
{
    reinterpret_cast<SchedulerT *>(context)->Schedule(h);
}
} // namespace Detail

/// @brief Forward declaration of default lock awaiter.
///
template <Concepts::Atomic AtomicT>
class LockAwaiter;

/// @brief Forward declaration of auto-lock awaiter.
///
template <Concepts::Atomic AtomicT>
class ScopedLockAwaiter;

/// @brief Async mutex implementation. Does not call mutex implementation if
/// lock cannot be immediately acquired, and puts coroutine asleep.
/// @tparam AtomicT Atomic primitive implementation.
///
template <Concepts::Atomic AtomicT>
class AsyncMutex
{
public:
    /// @brief Default constructor.
    ///
    AsyncMutex() noexcept = default;

    /// @brief Non-copyable.
    ///
    AsyncMutex(const AsyncMutex &) = delete;

    /// @brief Non-copyable.
    ///
    AsyncMutex &operator=(const AsyncMutex &) = delete;

    /// @brief Try to acquire without suspending.
    /// @return true if lock succeeded, false otherwise.
    ///
    bool TryLock() noexcept
    {
        auto expectedState = NotLocked;
        return m_lockStateOrQueue.compare_exchange_strong(
            expectedState,
            LockedNoQueue,
            std::memory_order::acquire,
            std::memory_order::relaxed);
    }

    /// @brief Try enqueuing coroutine for locking
    /// @param handleAwaiter Lock awaiter
    /// @returns true if enqueued and suspension is rquired.
    /// false if lock is acquired and coroutine is not enqueued.
    ///
    bool EnqueueForLock(Detail::LockAwaiterNode *handleAwaiter) noexcept
    {
        auto expectedState = m_lockStateOrQueue.load();
        for (;;)
        {
            // If mutex is not locked, try acquiring lock without queueing
            //
            if (expectedState == NotLocked)
            {
                if (m_lockStateOrQueue.compare_exchange_weak(
                        expectedState,
                        LockedNoQueue,
                        std::memory_order::acquire,
                        std::memory_order::relaxed))
                {
                    return false;
                }
            }
            // Mutex is locked, try enqueueing.
            //
            else
            {
                // Put LIFO node on top of waiters stack
                //
                handleAwaiter->Next =
                    reinterpret_cast<Detail::LockAwaiterNode *>(expectedState);
                if (m_lockStateOrQueue.compare_exchange_weak(
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

    /// @brief Unlock; if there is a waiter, transfer ownership to it (resume
    /// it).
    ///
    void Unlock() noexcept
    {
        // Steal current state, pretending there are no waiters.
        //
        auto expectedState =
            m_lockStateOrQueue.exchange(LockedNoQueue,
                                        std::memory_order::acquire);

        // If there were really no waiters, just set to unlocked state.
        //
        if (expectedState == LockedNoQueue)
        {
            m_lockStateOrQueue.store(NotLocked, std::memory_order::release);
            return;
        }

        // Scroll through the list to get the first awaiter.
        //
        auto *head = reinterpret_cast<Detail::LockAwaiterNode *>(expectedState);

        Detail::LockAwaiterNode *prev = nullptr;
        Detail::LockAwaiterNode *curr = head;
        while (curr->Next != nullptr)
        {
            prev = curr;
            curr = curr->Next;
        }

        // prev != nullptr means there are more awaiters than one
        //
        if (prev != nullptr)
        {
            prev->Next = nullptr;
            m_lockStateOrQueue.store(
                reinterpret_cast<Concepts::AtomicPrimitive>(head),
                std::memory_order::release);
        }
        else
        {
            m_lockStateOrQueue.store(LockedNoQueue, std::memory_order_release);
        }

        curr->Resume();
    }

    /// @brief Get awaitable: co_await mutex.LockAsync();
    ///
    LockAwaiter<AtomicT> LockAsync() noexcept
    {
        return LockAwaiter<AtomicT>{this};
    }

    /// @brief Get awaitable: co_await mutex.LockAsync(scheduler);
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    LockAwaiter<AtomicT> LockAsync(SchedulerT &sched) noexcept
    {
        return LockAwaiter<AtomicT>{this, sched};
    }

    /// @brief Get awaitable: co_await mutex.ScopedLockAsync();
    ///
    ScopedLockAwaiter<AtomicT> ScopedLockAsync() noexcept
    {
        return ScopedLockAwaiter<AtomicT>{this};
    }

    /// @brief Get awaitable: co_await mutex.ScopedLockAsync(scheduler);
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    ScopedLockAwaiter<AtomicT> ScopedLockAsync(SchedulerT &sched) noexcept
    {
        return ScopedLockAwaiter<AtomicT>{this, sched};
    }

private:
    static constexpr auto LockedNoQueue{0};
    static constexpr auto NotLocked{
        (std::numeric_limits<Concepts::AtomicPrimitive>::max)()};
    AtomicT m_lockStateOrQueue{NotLocked};
};

/// @brief Mutex awaiter common code - storage of async mutex and synchronized
/// wait list accessors.
/// @tparam AtomicT Atomic primitive implementation.
///
template <Concepts::Atomic AtomicT>
class LockAwaiterBase : protected Detail::LockAwaiterNode
{
protected:
    AsyncMutex<AtomicT> *m_mutex;

    /// @brief Costructor. Initializes storage.
    /// @param mutex AsyncMutex.
    ///
    LockAwaiterBase(AsyncMutex<AtomicT> *mutex) noexcept : m_mutex{mutex}
    {
    }
};

/// @brief Awaitable returned by AsyncMutex<AtomicT>::LockAsync()
/// @tparam AtomicT Atomic primitive implementation.
///
template <Concepts::Atomic AtomicT>
class LockAwaiter : LockAwaiterBase<AtomicT>
{
public:
    /// @brief Constructor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    ///
    LockAwaiter(AsyncMutex<AtomicT> *mutex) noexcept :
        LockAwaiterBase<AtomicT>{mutex}
    {
    }

    /// @brief Constructor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    /// @param scheduler Scheduler on which to resume this coroutine when lock
    /// is available.
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    LockAwaiter(AsyncMutex<AtomicT> *mutex, SchedulerT &scheduler) noexcept :
        LockAwaiterBase<AtomicT>{mutex}
    {
        Detail::LockAwaiterNode::HandleResumerFunc =
            Detail::ScheduleNextWaiter<SchedulerT>;

        Detail::LockAwaiterNode::HandleResumerFuncContext = &scheduler;
    }

    /// @brief Compiler contract: Try locking mutex quickly.
    /// @returns true if succeeded, false otherwise.
    ///
    bool await_ready() noexcept
    {
        // fast path: acquire without suspending
        return this->m_mutex->TryLock();
    }

    /// @brief Compiler contract: If we return false here,
    /// the coroutine will continue immediately;
    /// return true to suspend. We must handle the race between the fast path
    /// and enqueuing -> re-check before enqueuing.
    /// @returns true if locked quickly, false otherwise.
    ///
    template <Concepts::TaskImpl T, typename R>
    bool await_suspend(std::coroutine_handle<PromiseType<T, R>> h) noexcept
    {
        AwaiterBase::await_suspend(h);

        this->HandleToResume = h;

        return this->m_mutex->EnqueueForLock(this);
    }

    /// @brief Compiler contract: Resume action - do nothing, just restore
    /// AwaiterBase state.
    ///
    using AwaiterBase::await_resume;
};

/// @brief Lightweight RAII guard for use in coroutine functions.
/// @tparam AtomicT Atomic primitive implementation.
///
template <Concepts::Atomic AtomicT>
class ScopedLock
{
public:
    /// @brief Constructor. Initialized held mutex.
    ///
    ScopedLock(AsyncMutex<AtomicT> *mutex) noexcept : m_mutex{mutex}
    {
    }

    /// @brief Non-copyable.
    ///
    ScopedLock(const ScopedLock &) = delete;

    /// @brief Non-copyable.
    ///
    ScopedLock &operator=(const ScopedLock &) = delete;

    /// @brief Destructor. Releases held lock if any.
    ///
    ~ScopedLock() noexcept
    {
        if (m_lock)
        {
            m_mutex->Unlock();
        }
    }

    /// @brief Unlock held lock if any.
    ///
    void Unlock() noexcept
    {
        if (m_lock)
        {
            m_lock = false;
            m_mutex->Unlock();
        }
    }

private:
    AsyncMutex<AtomicT> *m_mutex;
    bool m_lock = true;
};

/// @brief Helper to create RAII guard after co_awaiting LockAsync():
/// usage: auto guard = co_await mutex.ScopedLockAsync();
/// @tparam AtomicT Atomic primitive implementation.
///
template <Concepts::Atomic AtomicT>
class ScopedLockAwaiter : LockAwaiterBase<AtomicT>
{
public:
    /// @brief Construtor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    ///
    ScopedLockAwaiter(AsyncMutex<AtomicT> *mutex) noexcept :
        LockAwaiterBase<AtomicT>{mutex}
    {
    }

    /// @brief Construtor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    /// @param scheduler Scheduler on which to resume this coroutine when lock
    /// is available.
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    ScopedLockAwaiter(AsyncMutex<AtomicT> *mutex,
                      SchedulerT &scheduler) noexcept :
        LockAwaiterBase<AtomicT>{mutex}
    {
        Detail::LockAwaiterNode::HandleResumerFunc =
            Detail::ScheduleNextWaiter<SchedulerT>;

        Detail::LockAwaiterNode::HandleResumerFuncContext = &scheduler;
    }

    /// @brief Compiler contract: Try locking mutex quickly.
    /// @returns true if succeeded, false otherwise.
    ///
    bool await_ready() noexcept
    {
        return this->m_mutex->TryLock();
    }

    /// @brief Compiler contract: Try locking mutex quickly once again.
    /// If failed, add awaiter to wait list.
    /// @returns true if locked quickly, false otherwise.
    ///
    template <Concepts::TaskImpl T, typename R>
    bool await_suspend(std::coroutine_handle<PromiseType<T, R>> h) noexcept
    {
        AwaiterBase::await_suspend(h);

        this->HandleToResume = h;

        return this->m_mutex->EnqueueForLock(this);
    }

    /// @brief Compiler contract: Resume action - restore AwaiterBase state
    /// and return RAII to AsyncMuex.
    ///
    [[nodiscard("Lock will be immeditely released!")]]
    decltype(auto) await_resume() noexcept
    {
        AwaiterBase::await_resume();
        return ScopedLock<AtomicT>{this->m_mutex};
    }
};

} // namespace Cortado

#endif // CORTADO_ASYNC_MUTEX_H
