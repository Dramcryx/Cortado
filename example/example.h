/// @file example.h
/// Platform-based definition of NativeMutexT and thread ID function.
///

#ifndef CORTADO_EXAMPLE_EXAMPLE_H
#define CORTADO_EXAMPLE_EXAMPLE_H

#ifdef __APPLE__

// Cortado
//
#include <Cortado/Common/MacOSMutex.h>

using NativeMutexT = Cortado::Common::MacOSMutex;

#define THREAD_ID ((long)pthread_self())

#elif defined(_WIN32)

// Cortado
//
#include <Cortado/Common/Win32Mutex.h>

using NativeMutexT = Cortado::Common::Win32Mutex;

#define THREAD_ID GetCurrentThreadId()

#else

// Cortado
//
#include <Cortado/Common/LinuxMutex.h>

using NativeMutexT = Cortado::Common::LinuxMutex;

#define THREAD_ID ((long)pthread_self())

#endif

#endif // CORTADO_EXAMPLE_EXAMPLE_H
