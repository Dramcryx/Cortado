/// @brief DefaultTaskImpl.h
/// A default task implementation for Win32 and Mac.
///

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

// Cortado
//
#include <Cortado/Common/STLCoroutineScheduler.h>

namespace Cortado
{
using DefaultScheduler = Common::STLCoroutineScheduler;
} // namespace Cortado

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

/// @brief Default implementation with platform-default scheduler,
/// STL allocator, atomic and exception.
///
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
