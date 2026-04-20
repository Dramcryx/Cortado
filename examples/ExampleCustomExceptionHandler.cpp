/// @file ExampleCustomExceptionHandler.cpp
/// Example of customizing an exception handler with a non-STL exception type.
///
/// This demonstrates how to replace STLExceptionHandler with your own
/// error type that does not derive from std::exception. Useful for
/// projects that avoid C++ exceptions or use a domain-specific error model.
///

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/DefaultTaskImpl.h>

// STL
//
#include <iostream>
#include <string>

/// @brief A custom error type that is not derived from std::exception.
///
struct AppError
{
    int Code;
    std::string Message;
};

/// @brief Custom exception handler that catches and rethrows AppError.
///
/// The handler stores the last thrown AppError in a thread_local variable.
/// `Catch()` is called inside `unhandled_exception()` and must return the
/// stored exception. `Rethrow()` is called from `Get()` and re-throws it.
///
struct AppErrorHandler
{
    /// @brief The exception type stored in coroutine promise storage.
    ///
    using Exception = AppError;

    /// @brief Called by the promise's `unhandled_exception()`.
    /// Must capture the in-flight exception and return it.
    ///
    static AppError Catch()
    {
        try
        {
            throw; // re-throw current exception to catch it as AppError
        }
        catch (const AppError &e)
        {
            return e;
        }
        catch (...)
        {
            // Fallback for unexpected types — map to a generic error.
            return AppError{-1, "Unknown error"};
        }
    }

    /// @brief Called by the promise's `Get()` when stored value is an error.
    /// @param e The stored error.
    ///
    static void Rethrow(AppError &&e)
    {
        throw e;
    }
};

/// @brief Compose a TaskImpl that uses AppErrorHandler instead of
/// STLExceptionHandler. Everything else stays default.
///
struct TaskImplWithCustomExceptionHandler :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLCoroutineAllocator,
    AppErrorHandler,
    Cortado::DefaultScheduler
{
    using Event = Cortado::DefaultEvent;
};

template <typename T = void>
using Task = Cortado::Task<T, TaskImplWithCustomExceptionHandler>;

/// @brief A coroutine that succeeds.
///
Task<int> SuccessAsync()
{
    co_await Cortado::ResumeBackground();
    co_return 42;
}

/// @brief A coroutine that fails with an AppError.
///
Task<int> FailAsync()
{
    co_await Cortado::ResumeBackground();
    throw AppError{404, "Resource not found"};
    co_return 0; // never reached
}

int main()
{
    // 1) Successful path
    //
    std::cout << "SuccessAsync returned: " << SuccessAsync().Get() << "\n";

    // 2) Error path — Get() rethrows AppError
    //
    try
    {
        FailAsync().Get();
    }
    catch (const AppError &e)
    {
        std::cout << "Caught AppError { Code=" << e.Code
                  << ", Message=\"" << e.Message << "\" }\n";
    }

    return 0;
}
