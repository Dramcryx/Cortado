/// @file StandardAsyncStackTLS.h
/// Default thread-local storage provider for async stack frames.
///

#ifndef CORTADO_COMMON_STANDARD_ASYNC_STACK_TLS_H
#define CORTADO_COMMON_STANDARD_ASYNC_STACK_TLS_H

namespace Cortado::Common
{

/// @brief Standard TLS provider using C++ thread_local.
/// Suitable for environments with native thread_local support.
///
struct StandardAsyncStackTLS
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

private:
    static void*& GetImpl()
    {
        static thread_local void *current = nullptr;
        return current;
    }
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_STANDARD_ASYNC_STACK_TLS_H
