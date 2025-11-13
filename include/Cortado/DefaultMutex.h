/// @file DefaultMutex.h
/// Platform-based definition of DefaultMutex.
///

#ifndef CORTADO_DEFAULT_MUTEX_H
#define CORTADO_DEFAULT_MUTEX_H

#ifdef __APPLE__

// Cortado
//
#include <Cortado/Common/MacOSMutex.h>

namespace Cortado
{
using DefaultMutex = Cortado::Common::MacOSMutex;
} // namespace Cortado

#elif defined(_WIN32)

// Cortado
//
#include <Cortado/Common/Win32Mutex.h>

namespace Cortado
{
using DefaultMutex = Cortado::Common::Win32Mutex;
} // namespace Cortado

#elif defined(__linux__)

// Cortado
//
#include <Cortado/Common/LinuxMutex.h>

namespace Cortado
{
using DefaultMutex = Cortado::Common::LinuxMutex;
} // namespace Cortado

#endif

#endif // CORTADO_DEFAULT_MUTEX_H
