/// @file example.h
/// Platform-based definition of NativeMutexT and thread ID function.
///

#ifndef CORTADO_EXAMPLE_EXAMPLE_H
#define CORTADO_EXAMPLE_EXAMPLE_H

#ifdef _WIN32

#define THREAD_ID GetCurrentThreadId()

#else // !_WIN32

#define THREAD_ID ((long)pthread_self())

#endif

#endif // CORTADO_EXAMPLE_EXAMPLE_H
