/// @file DefaultScheduler.h
/// Platform-based definition of DefaultScheduler.
///

#ifndef CORTADO_DEFAULT_SCHEDULER_H
#define CORTADO_DEFAULT_SCHEDULER_H

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

#elif defined(_POSIX_VERSION)

// Cortado
//
#include <Cortado/Common/PosixCoroutineScheduler.h>

namespace Cortado
{
using DefaultScheduler = Common::PosixCoroutineScheduler;
} // namespace Cortado

#else

namespace Cortado
{
struct NoScheduler
{
};
using DefaultScheduler = NoScheduler;
} // namespace Cortado

#endif

#endif // CORTADO_DEFAULT_SCHEDULER_H
