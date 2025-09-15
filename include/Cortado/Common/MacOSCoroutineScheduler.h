/// @file MacOSCoroutineScheduler.h
/// Implementation of a default async runtime using Grand Central Dispatch
/// (GCD).
///

#ifndef CORTADO_COMMON_MACOS_COROUTINE_SCHEDULER_H
#define CORTADO_COMMON_MACOS_COROUTINE_SCHEDULER_H

#ifdef __APPLE__

// macOS
//
#include <dispatch/dispatch.h>

// STL
//
#include <coroutine>

namespace Cortado::Common
{

/// @brief Simple coroutine scheduler on a GCD-provided dispatcher.
///
struct MacOSCoroutineScheduler
{
    /// @brief Concept contract: Schedules coroutine in a different thread.
    /// @param h Coroutine to schedule.
    ///
    void Schedule(std::coroutine_handle<> h)
    {
        dispatch_async_f(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0),
                         h.address(),
                         WorkCallback);
    }

    /// @brief Concept contract: Get app-global scheduler instance.
    ///
    static MacOSCoroutineScheduler &GetDefaultBackgroundScheduler()
    {
        static MacOSCoroutineScheduler sched;
        return sched;
    }

private:
    /// @brief GCD contract: Function that restores coroutine handle and
    /// resumes it.
    ///
    static void WorkCallback(void *context)
    {
        auto h = std::coroutine_handle<>::from_address(context);
        h();
    }
};

} // namespace Cortado::Common

#endif // __APPLE__

#endif
