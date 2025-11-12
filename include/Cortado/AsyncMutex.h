/// @file AsyncMutex.h
/// Implementation of async mutex.
///

#ifndef CORTADO_ASYNC_MUTEX_H
#define CORTADO_ASYNC_MUTEX_H

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Concepts/CoroutineScheduler.h>
#include <Cortado/Concepts/Mutex.h>

// STL
//
#include <mutex> // for std::lock_guard

namespace Cortado
{
namespace Detail
{
/// @brief Base for all of the mutex awaiters, for storing in a linked-list.
///
struct LockAwaiterNode : AwaiterBase
{
    LockAwaiterNode *Prev{nullptr};
    std::coroutine_handle<> HandleToResume{nullptr};
    void (*HandleResumerFunc)(std::coroutine_handle<>, void*) = nullptr;
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

/// @brief Helper function to attach new awaiter node to list anchor.
/// @param anchorNode List anchor node, stored in mutex.
/// @param nodeToInsert New awaiter node.
///
inline void InsertAwaiter(Detail::LockAwaiterNode *anchorNode,
                          Detail::LockAwaiterNode *nodeToInsert)
{
    // Anchor node points to nothing or to itself - list is empty.
    //
    if (anchorNode->Prev == nullptr || anchorNode->Prev == anchorNode)
    {
        anchorNode->Next = nodeToInsert;
        anchorNode->Prev = nodeToInsert;
        nodeToInsert->Next = anchorNode;
        nodeToInsert->Prev = anchorNode;
    }
    else
    {
        auto *last = anchorNode->Prev;
        last->Next = nodeToInsert;
        nodeToInsert->Prev = last;
        nodeToInsert->Next = anchorNode;
        nodeToInsert->Next->Prev = nodeToInsert;
    }
}
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
template <Concepts::Atomic AtomicT, Concepts::Mutex MutexT>
class LockAwaiter;

/// @brief Forward declaration of auto-lock awaiter.
///
template <Concepts::Atomic AtomicT, Concepts::Mutex MutexT>
class ScopedLockAwaiter;

/// @brief Async mutex implementation. Does not call mutex implementation if
/// lock cannot be immediately acquired, and puts coroutine asleep.
/// @tparam AtomicT Atomic primitive implementation.
/// @tparam MutexT Mutex implementation.
///
template <Concepts::Atomic AtomicT, Concepts::Mutex MutexT>
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
        Concepts::AtomicPrimitive expected = 0;
        return m_lock.compare_exchange_strong(expected, 1);
    }

    /// @brief Unlock; if there is a waiter, transfer ownership to it (resume
    /// it).
    ///
    void Unlock() noexcept
    {
        Detail::LockAwaiterNode* next = nullptr;
        {
            std::lock_guard lk(m_waitersMutex);
            if (m_waiters.Next != nullptr && m_waiters.Next != &m_waiters)
            {
                next = m_waiters.Next;
                m_waiters.Next = m_waiters.Next->Next;
                m_waiters.Next->Prev = &m_waiters;
                // keep m_lock as true because ownership is transferred
            }
            else
            {
                // no waiter: release the lock
                m_lock.store(0);
            }
        }

        if (next != nullptr)
        {
            next->Resume();
        }
    }

    /// @brief Get awaitable: co_await mutex.LockAsync();
    ///
    LockAwaiter<AtomicT, MutexT> LockAsync() noexcept
    {
        return LockAwaiter<AtomicT, MutexT>{this, m_waitersMutex, m_waiters};
    }

    /// @brief Get awaitable: co_await mutex.LockAsync(scheduler);
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    LockAwaiter<AtomicT, MutexT> LockAsync(SchedulerT &sched) noexcept
    {
        return LockAwaiter<AtomicT, MutexT>{this,
                                            m_waitersMutex,
                                            m_waiters,
                                            sched};
    }

    /// @brief Get awaitable: co_await mutex.ScopedLockAsync();
    ///
    ScopedLockAwaiter<AtomicT, MutexT> ScopedLockAsync() noexcept
    {
        return ScopedLockAwaiter<AtomicT, MutexT>{this,
                                                  m_waitersMutex,
                                                  m_waiters};
    }

    /// @brief Get awaitable: co_await mutex.ScopedLockAsync(scheduler);
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    ScopedLockAwaiter<AtomicT, MutexT> ScopedLockAsync(SchedulerT& sched) noexcept
    {
        return ScopedLockAwaiter<AtomicT, MutexT>{this,
                                                  m_waitersMutex,
                                                  m_waiters,
                                                  sched};
    }


private:
    AtomicT m_lock{0};
    MutexT m_waitersMutex;
    Detail::LockAwaiterNode m_waiters;
};

/// @brief Mutex awaiter common code - storage of async mutex and synchronized
/// wait list accessors.
/// @tparam AtomicT Atomic primitive implementation.
/// @tparam MutexT Mutex implementation.
///
template <Concepts::Atomic AtomicT, Concepts::Mutex MutexT>
class LockAwaiterBase : protected Detail::LockAwaiterNode
{
protected:
    AsyncMutex<AtomicT, MutexT> *m_mutex;
    MutexT &m_waitersMutex;
    Detail::LockAwaiterNode &m_waiters;

    /// @brief Costructor. Initializes storage.
    /// @param mutex AsyncMutex.
    /// @param waitersMutex Synchronizer for waiters list.
    /// @param waiters Waiters list.
    ///
    LockAwaiterBase(AsyncMutex<AtomicT, MutexT> *mutex,
                    MutexT &waitersMutex,
                    Detail::LockAwaiterNode &waiters) noexcept :
        m_mutex{mutex}, m_waitersMutex{waitersMutex}, m_waiters{waiters}
    {
    }


};

/// @brief Awaitable returned by AsyncMutex<AtomicT, MutexT>::LockAsync()
/// @tparam AtomicT Atomic primitive implementation.
/// @tparam MutexT Mutex implementation.
///
template <Concepts::Atomic AtomicT, Concepts::Mutex MutexT>
class LockAwaiter : LockAwaiterBase<AtomicT, MutexT>
{
public:
    /// @brief Constructor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    /// @param waitersMutex Synchronizer for waiters list.
    /// @param waiters Waiters list.
    ///
    LockAwaiter(AsyncMutex<AtomicT, MutexT> *mutex,
                MutexT &waitersMutex,
                Detail::LockAwaiterNode &waiters) noexcept :
        LockAwaiterBase<AtomicT, MutexT>{mutex, waitersMutex, waiters}
    {
    }

    /// @brief Constructor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    /// @param waitersMutex Synchronizer for waiters list.
    /// @param waiters Waiters list.
    /// @param scheduler Scheduler on which to resume this coroutine when lock
    /// is available.
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    LockAwaiter(AsyncMutex<AtomicT, MutexT> *mutex,
                      MutexT &waitersMutex,
                      Detail::LockAwaiterNode &waiters,
                      SchedulerT &scheduler) noexcept :
        LockAwaiterBase<AtomicT, MutexT>{mutex, waitersMutex, waiters}
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

        // attempt to acquire again (race with unlock)
        if (this->m_mutex->TryLock())
        {
            return false; // don't suspend, continue immediately as owner
        }

        {
            std::lock_guard lk(this->m_waitersMutex);
            // re-check under queue lock to avoid a race with unlock
            if (this->m_mutex->TryLock())
            {
                return false;
            }

            Detail::LockAwaiterNode::HandleToResume = h;

            InsertAwaiter(&this->m_waiters, this);
            // suspended; will be resumed by unlock()
        }

        return true;
    }

    /// @brief Compiler contract: Resume action - do nothing, just restore
    /// AwaiterBase state.
    ///
    using AwaiterBase::await_resume;
};

/// @brief Lightweight RAII guard for use in coroutine functions.
/// @tparam AtomicT Atomic primitive implementation.
/// @tparam MutexT Mutex implementation.
///
template <Concepts::Atomic AtomicT, Concepts::Mutex MutexT>
class ScopedLock
{
public:
    /// @brief Constructor. Initialized held mutex.
    ///
    ScopedLock(AsyncMutex<AtomicT, MutexT> *mutex) noexcept : m_mutex{mutex}
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
    AsyncMutex<AtomicT, MutexT> *m_mutex;
    bool m_lock = true;
};

/// @brief Helper to create RAII guard after co_awaiting LockAsync():
/// usage: auto guard = co_await mutex.ScopedLockAsync();
/// @tparam AtomicT Atomic primitive implementation.
/// @tparam MutexT Mutex implementation.
///
template <Concepts::Atomic AtomicT, Concepts::Mutex MutexT>
class ScopedLockAwaiter : LockAwaiterBase<AtomicT, MutexT>
{
public:
    /// @brief Construtor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    /// @param waitersMutex Synchronizer for waiters list.
    /// @param waiters Waiters list.
    ///
    ScopedLockAwaiter(AsyncMutex<AtomicT, MutexT> *mutex,
                      MutexT &waitersMutex,
                      Detail::LockAwaiterNode &waiters) noexcept :
        LockAwaiterBase<AtomicT, MutexT>{mutex, waitersMutex, waiters}
    {
    }

    /// @brief Construtor. Forwards arguments to awaiter base.
    /// @param mutex AsyncMutex.
    /// @param waitersMutex Synchronizer for waiters list.
    /// @param waiters Waiters list.
    /// @param scheduler Scheduler on which to resume this coroutine when lock is available.
    ///
    template <Concepts::CoroutineScheduler SchedulerT>
    ScopedLockAwaiter(AsyncMutex<AtomicT, MutexT> *mutex,
                      MutexT &waitersMutex,
                      Detail::LockAwaiterNode &waiters,
                      SchedulerT& scheduler) noexcept :
        LockAwaiterBase<AtomicT, MutexT>{mutex, waitersMutex, waiters}
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

        if (this->m_mutex->TryLock())
        {
            return false;
        }
        {
            std::lock_guard lk(this->m_waitersMutex);
            if (this->m_mutex->TryLock())
            {
                return false;
            }

            Detail::LockAwaiterNode::HandleToResume = h;

            InsertAwaiter(&this->m_waiters, this);
        }

        return true;
    }

    /// @brief Compiler contract: Resume action - restore AwaiterBase state
    /// and return RAII to AsyncMuex.
    ///
    [[nodiscard("Lock will be immeditely released!")]]
    decltype(auto) await_resume() noexcept
    {
        AwaiterBase::await_resume();
        return ScopedLock<AtomicT, MutexT>{this->m_mutex};
    }
};

} // namespace Cortado

#endif // CORTADO_ASYNC_MUTEX_H
