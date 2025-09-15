/// @file Win32CoroutineScheduler.h
/// Implementation of a default async runtime using Win32 Threadpool API.
///

#ifndef CORTADO_COMMON_WIN32_COROUTINE_SCHEDULER_H
#define CORTADO_COMMON_WIN32_COROUTINE_SCHEDULER_H

#ifdef _WIN32

// Win32
//
#include <threadpoolapiset.h>

// STL
//
#include <coroutine>

namespace Cortado::Common
{

/// @brief Simple coroutine scheduler on a Win32 Threadpool.
///
struct Win32CoroutineScheduler
{
    /// @brief Concept contract: Schedules coroutine in a different thread.
    /// @param h Coroutine to schedule.
    ///
    void Schedule(std::coroutine_handle<> h)
    {
        // Create a thread pool work object
        PTP_WORK work =
            CreateThreadpoolWork(WorkCallback, h.address(), nullptr);

        // Submit work to the pool
        SubmitThreadpoolWork(work);

        // Close the work object (does NOT cancel execution; just releases our
        // handle)
        CloseThreadpoolWork(work);
    }

    /// @brief Concept contract: Get app-global scheduler instance.
    ///
    static Win32CoroutineScheduler &GetDefaultBackgroundScheduler()
    {
        static Win32CoroutineScheduler sched;
        return sched;
    }

private:
    /// @brief Win32 contract: Function that restores coroutine handle and
    /// resumes it.
    ///
    static void CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE,
                                      PVOID Context,
                                      PTP_WORK)
    {
        auto h = std::coroutine_handle<>::from_address(Context);
        h();
    }
};

} // namespace Cortado::Common

#endif // _WIN32

#endif
