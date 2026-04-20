/// @file DefaultEvent.h
/// Platform-based definition of DefaultEvent.
///

#ifndef CORTADO_DEFAULT_EVENT_H
#define CORTADO_DEFAULT_EVENT_H

// Cortado
//
#include <Cortado/AsyncEvent.h>

#ifdef _WIN32

// Cortado
//
#include <Cortado/Common/Win32FutexLikeAtomic.h>

namespace Cortado
{
using DefaultEvent = AsyncEvent<Common::Win32FutexLikeAtomic>;
} // namespace Cortado

#elif defined(__APPLE__)

// Cortado
//
#include <Cortado/Common/MacOSFutexLikeAtomic.h>

namespace Cortado
{
using DefaultEvent = AsyncEvent<Cortado::Common::MacOSFutexLikeAtomic>;
} // namespace Cortado

#elif defined(__linux__)

// Cortado
//
#include <Cortado/Common/LinuxFutexLikeAtomic.h>

namespace Cortado
{
using DefaultEvent = AsyncEvent<Cortado::Common::LinuxFutexLikeAtomic>;
} // namespace Cortado

#else

namespace Cortado
{
struct NoEvent
{
};
using DefaultEvent = NoEvent;
} // namespace Cortado

#endif

#endif // CORTADO_DEFAULT_EVENT_H
