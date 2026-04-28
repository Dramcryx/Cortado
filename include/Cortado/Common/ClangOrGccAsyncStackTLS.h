/// @file ClangOrGccAsyncStackTLS.h
/// TLS provider for Clang/GCC using __builtin_return_address.
/// Force-inlined into initial_suspend so the captured address points
/// into the user coroutine's bootstrap and resolves to the user
/// coroutine's symbol via the platform symbolizer.
///

#ifndef CORTADO_COMMON_CLANG_OR_GCC_ASYNC_STACK_TLS_H
#define CORTADO_COMMON_CLANG_OR_GCC_ASYNC_STACK_TLS_H

#if !defined(__clang__) && !defined(__GNUC__)
#error "ClangOrGccAsyncStackTLS requires Clang or GCC"
#endif

namespace Cortado::Common
{

/// @brief TLS provider using C++ thread_local storage and the
/// __builtin_return_address compiler builtin for return-address capture.
///
struct ClangOrGccAsyncStackTLS
{
    /// @brief Get the current async stack frame pointer for this thread.
    /// @returns The stored pointer, or nullptr.
    ///
    inline static void *Get()
    {
        return GetImpl();
    }

    /// @brief Set the current async stack frame pointer for this thread.
    /// @param p The pointer to store.
    ///
    inline static void Set(void *p)
    {
        GetImpl() = p;
    }

    /// @brief Capture the return address of the caller. Force-inlined so
    /// the captured address belongs to the caller of this function (i.e.
    /// initial_suspend), which is itself inlined into the user coroutine
    /// bootstrap on Clang/GCC.
    ///
    __attribute__((always_inline)) inline
    static void *CaptureReturnAddress()
    {
        return __builtin_return_address(0);
    }

private:
    static void*& GetImpl()
    {
        static thread_local void *current = nullptr;
        return current;
    }
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_CLANG_OR_GCC_ASYNC_STACK_TLS_H
