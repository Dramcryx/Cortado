/// @file STLCoroutineScheduler.h
/// Silly and slow coroutine scheduler for minimal Linux support
///

#ifndef CORTADO_COMMON_STL_COROUTINE_SCHEDULER_H
#define CORTADO_COMMON_STL_COROUTINE_SCHEDULER_H

// STL
//
#include <coroutine>
#include <thread>

namespace Cortado::Common
{

/// @brief A very silly scheduler for native example demonstration on Linux.
///
struct STLCoroutineScheduler
{
    /// @brief Concept contract: Schedules coroutine in a different thread.
    /// @param h Coroutine to schedule.
    ///
    void Schedule(std::coroutine_handle<> h)
    {
        std::thread{[=]
                    {
                        h();
                    }}
            .detach();
    }

    /// @brief Concept contract: Get app-global scheduler instance.
    ///
    static STLCoroutineScheduler &GetDefaultBackgroundScheduler()
    {
        static STLCoroutineScheduler sched;
        return sched;
    }
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_STL_COROUTINE_SCHEDULER_H
