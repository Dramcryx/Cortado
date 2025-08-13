#ifndef CORTADO_DEFAULT_TASK_IMPL_H
#define CORTADO_DEFAULT_TASK_IMPL_H

// Cortado
//
#include <Cortado/Common/STLAtomic.h>
#include <Cortado/Common/STLCoroutineAllocator.h>
#include <Cortado/Common/STLExceptionHandler.h>

#ifdef __APPLE__

// Cortado
//
#include <Cortado/Common/MacOSCoroutineScheduler.h>

namespace Cortado
{
using DefaultScheduler = Common::MacOSCoroutineScheduler;
} // namespace Cortado

#elif defined(_WIN32)

// Cortado
//
#include <Cortado/Common/Win32CoroutineScheduler.h>

// Win32
//
#include <processthreadsapi.h>

namespace Cortado
{
using DefaultScheduler = Common::Win32CoroutineScheduler;
} // namespace Cortado

#else
#error "Linux is not implemented yet"
#endif

#if defined(_WIN32)

// Cortado
//
#include <Cortado/Common/Win32Event.h>

namespace Cortado
{
using DefaultEvent = Common::Win32Event;
} // namespace Cortado

#elif defined(_POSIX_VERSION)

// Cortado
//
#include <Cortado/Common/PosixEvent.h>

namespace Cortado
{
using DefaultEvent = Common::PosixEvent;
} // namespace Cortado

#endif

namespace Cortado
{

struct DefaultTaskImpl :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    DefaultScheduler
{
    using Event = DefaultEvent;
};

} // namespace Cortado

#endif
