/// @file DefaultEvent.h
/// Platform-based definition of DefaultEvent.
///

#ifndef CORTADO_DEFAULT_EVENT_H
#define CORTADO_DEFAULT_EVENT_H

#ifdef _WIN32

// Cortado
//
#include <Cortado/Common/Win32Event.h>

namespace Cortado
{
using DefaultEvent = Cortado::Common::Win32Event;
} // namespace Cortado

#elif defined(_POSIX_VERSION) || defined(__APPLE__)

// Cortado
//
#include <Cortado/Common/PosixEvent.h>

namespace Cortado
{
using DefaultEvent = Cortado::Common::PosixEvent;
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
