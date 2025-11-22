/// @file DefaultThreadId.h
/// Platform-based definition of thread ID function.
///

#ifndef CORTADO_EXAMPLES_DEFAULT_THREAD_ID_H
#define CORTADO_EXAMPLES_DEFAULT_THREAD_ID_H

#ifdef _WIN32

#define THREAD_ID GetCurrentThreadId()

#else // !_WIN32

#define THREAD_ID ((long)pthread_self())

#endif

#endif // CORTADO_EXAMPLES_DEFAULT_THREAD_ID_H
