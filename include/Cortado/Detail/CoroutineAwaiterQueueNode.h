/// @file CoroutineAwaiterQueueNode.h
/// Common class of an async queue for AsyncMutex and AsyncEvent
///

#ifndef CORTADO_DETAIL_COROUTINE_AWAITER_QUEUE_NODE_H
#define CORTADO_DETAIL_COROUTINE_AWAITER_QUEUE_NODE_H

// Cortado
//
#include <Cortado/AwaiterBase.h>
#include <Cortado/Concepts/CoroutineScheduler.h>

// STL
//
#include <coroutine>

namespace Cortado::Detail
{
/// @brief Node in a singly-linked list of awaiters waiting on an async primitive.
/// Used by both AsyncMutex and AsyncEvent to maintain the queue of waiting coroutines.
///
struct CoroutineAwaiterQueueNode : AwaiterBase
{
    /// @brief Handle of the coroutine to resume when the primitive is ready.
    /// This value is mandatory and is set by the await_suspend of the awaiter that enqueues itself.
    ///
    std::coroutine_handle<> HandleToResume{nullptr};

    /// @brief Type-erased function pointer and context for resuming the next waiter.
    /// If HandleResumerFunc is set, it will be called with the handle of the coroutine to resume and the context when resuming.
    ///
    void (*HandleResumerFunc)(std::coroutine_handle<>, void *) = nullptr;

    /// @brief Type-erased context pointer for the resumer function, e.g. to hold a pointer to a scheduler.
    ///
    void *HandleResumerFuncContext = nullptr;

    /// @brief Pointer to the next awaiter in the queue.
    ///
    CoroutineAwaiterQueueNode *Next{nullptr};

    /// @brief Resume the coroutine associated with this awaiter.
    /// If a resumer function is set, use it to resume the coroutine (e.g. to schedule it on a specific scheduler).
    /// Otherwise, resume the coroutine directly.
    ///
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
} // namespace Cortado::Detail

#endif // CORTADO_DETAIL_COROUTINE_AWAITER_QUEUE_NODE_H
