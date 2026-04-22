/// @file ExampleAsyncStackTrace.cpp
/// Example of async stack tracing with Cortado.
///
/// This demonstrates how to define a TaskImpl with async stack tracing
/// enabled, and how to walk the async call stack from within a coroutine
/// or from a plain synchronous function.
///

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Common/StandardAsyncStackTLS.h>
#include <Cortado/DefaultTaskImpl.h>
#include <Cortado/Detail/AsyncStackFrame.h>

// STL
//
#include <iostream>

/// @brief A TaskImpl that enables async stack tracing using standard
/// thread_local storage. Inherits everything else from DefaultTaskImpl.
///
struct TaskImplWithAsyncStack :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    Cortado::DefaultScheduler
{
    using Event = Cortado::DefaultEvent;
    using AsyncStackTLSProvider = Cortado::Common::StandardAsyncStackTLS;
};

template <typename T = void>
using Task = Cortado::Task<T, TaskImplWithAsyncStack>;

using AsyncFrame =
    Cortado::Detail::AsyncStackFrame<Cortado::Common::StandardAsyncStackTLS>;

/// @brief A plain synchronous function that dumps the async stack.
/// This simulates what a DBG_ASSERT implementation would do —
/// no knowledge of coroutine handles or TaskImpl needed.
///
void DumpAsyncStack(const char *label)
{
    std::cout << "=== Async stack at " << label << " ===" << std::endl;

    int depth = 0;
    Cortado::Detail::WalkCurrentAsyncStack<
        Cortado::Common::StandardAsyncStackTLS>(
        [&](const AsyncFrame &frame) -> bool
        {
            std::cout << "  [" << depth << "] frame @ " << &frame << std::endl;
            ++depth;
            return true;
        });

    if (depth == 0)
    {
        std::cout << "  (empty)" << std::endl;
    }

    std::cout << std::endl;
}

/// @brief Innermost coroutine — dumps the stack from here.
///
Task<int> GrandChild()
{
    DumpAsyncStack("GrandChild");
    co_return 42;
}

/// @brief Middle coroutine.
///
Task<int> Child()
{
    auto result = co_await GrandChild();
    co_return result;
}

/// @brief Outermost coroutine.
///
Task<int> Parent()
{
    auto result = co_await Child();
    co_return result;
}

int main()
{
    DumpAsyncStack("before any coroutine");

    auto task = Parent();
    int result = task.Get();

    std::cout << "Result: " << result << std::endl;

    DumpAsyncStack("after all coroutines completed");

    return 0;
}
