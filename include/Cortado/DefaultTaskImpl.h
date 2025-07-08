#ifndef CORTADO_DEFAULT_TASK_IMPL_H
#define CORTADO_DEFAULT_TASK_IMPL_H

// Cortado
//
#include <Cortado/Common/STLAtomicIncDec.h>
#include <Cortado/Common/STLCoroutineAllocator.h>
#include <Cortado/Common/STLExceptionHandler.h>

#ifdef __APPLE__

// Cortado
//
#include <Cortado/Common/MacOSCoroutineScheduler.h>
#include <Cortado/Common/MacOSAtomicCompareExchange.h>

// POSIX
//
#include <pthread.h>

namespace Cortado
{
using DefaultScheduler = Common::MacOSCoroutineScheduler;
using AtomicCompareExchange = Common::MacOSAtomicCompareExchange;
} // namespace Cortado

#ifndef CORTADO_DEFAULT_YIELD
#define CORTADO_DEFAULT_YIELD pthread_yield_np
#endif

#elif defined(_WIN32)

// Cortado
//
#include <Cortado/Common/Win32CoroutineScheduler.h>
#include <Cortado/Common/Win32AtomicCompareExchange.h>

// Win32
//
#include <processthreadsapi.h>

namespace Cortado
{
using DefaultScheduler = Common::Win32CoroutineScheduler;
using AtomicCompareExchange = Common::Win32AtomicCompareExchange;
} // namespace Cortado

#ifndef CORTADO_DEFAULT_YIELD
#define CORTADO_DEFAULT_YIELD YieldProcessor
#endif

#else
#error "Linux is not implemented yet"
#endif

namespace Cortado
{

struct DefaultTaskImpl :
    Cortado::Common::STLAtomicIncDec,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    DefaultScheduler,
    AtomicCompareExchange
{
    inline static void YieldCurrentThread()
    {
        CORTADO_DEFAULT_YIELD();
    }
};

} // namespace Cortado

#endif
