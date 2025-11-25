/// @file ExampleDefaultTaskImpl.h
/// Platform-based definition of thread ID function.
///

#ifndef CORTADO_EXAMPLES_EXAMPLE_DEFAULT_TASK_IMPL_H
#define CORTADO_EXAMPLES_EXAMPLE_DEFAULT_TASK_IMPL_H

#ifdef _WIN32

#define THREAD_ID GetCurrentThreadId()

#else // !_WIN32

#define THREAD_ID ((long)pthread_self())

#endif

#endif // CORTADO_EXAMPLES_EXAMPLE_DEFAULT_TASK_IMPL_H
