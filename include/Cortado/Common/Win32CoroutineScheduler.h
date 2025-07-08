#ifndef CORTADO_COMMON_COROUTINE_SCHEDULERS_WIN32_COROUTINE_SCHEDULER_H
#define CORTADO_COMMON_COROUTINE_SCHEDULERS_WIN32_COROUTINE_SCHEDULER_H

// Win32
//
#include <threadpoolapiset.h>

// STL
//
#include <coroutine>

namespace Cortado::Common::CoroutineSchedulers
{

struct Win32CoroutineScheduler
{
    void Schedule(std::coroutine_handle<> h)
    {
        // Create a thread pool work object
        PTP_WORK work = CreateThreadpoolWork(WorkCallback, h.address(), nullptr);

        // Submit work to the pool
        SubmitThreadpoolWork(work);

        // Close the work object (does NOT cancel execution; just releases our handle)
        CloseThreadpoolWork(work);
    }

    static Win32CoroutineScheduler& GetDefaultBackgroundScheduler()
    {
        static Win32CoroutineScheduler sched;
        return sched;
    }

private:
    static void CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK)
    {
        auto h = std::coroutine_handle<>::from_address(Context);
        h();
    }
};

} // namespace Cortado::Common::CoroutineSchedulers

#endif
