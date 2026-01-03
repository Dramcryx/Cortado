/// @file CoroutineAwaiterQueueNode.h
/// Common class of an async queue for AsyncMutex and AsyncEvent
///

#ifndef CORTADO_DETAIL_COROUTINE_AWAITER_QUEUE_NODE_H
#define CORTADO_DETAIL_COROUTINE_AWAITER_QUEUE_NODE_H

// Cortado
//
#include <Cortado/AwaiterBase.h>
#include <Cortado/Concepts/CoroutineScheduler.h>

namespace Cortado::Detail
{
struct CoroutineAwaiterQueueNode : AwaiterBase
{
    std::coroutine_handle<> HandleToResume{nullptr};
    void (*HandleResumerFunc)(std::coroutine_handle<>, void *) = nullptr;
    void *HandleResumerFuncContext = nullptr;
    CoroutineAwaiterQueueNode *Next{nullptr};

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
