#ifndef CORTADO_COMMON_MACOS_COROUTINE_SCHEDULER_H
#define CORTADO_COMMON_MACOS_COROUTINE_SCHEDULER_H

// macOS
//
#include <dispatch/dispatch.h>

// STL
//
#include <coroutine>

namespace Cortado::Common
{

struct MacOSCoroutineScheduler
{
    void Schedule(std::coroutine_handle<> h)
    {
        dispatch_async_f(
            dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0),
            h.address(),
            WorkCallback);
    }

    static MacOSCoroutineScheduler& GetDefaultBackgroundScheduler()
    {
        static MacOSCoroutineScheduler sched;
        return sched;
    }

private:
    static void WorkCallback(void* context)
    {
        auto h = std::coroutine_handle<>::from_address(context);
        h();
    }
};

} // namespace Cortado::Common

#endif
