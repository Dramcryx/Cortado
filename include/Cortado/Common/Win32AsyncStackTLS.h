/// @file Win32AsyncStackTLS.h
/// Win32-specific TLS storage.
/// Due to unclear bug, C++ native thread_local
/// didn't work only on Windows CI and only in remote.
/// 3 different local machines with 3 different CPUs
/// produced no issues.
///

#ifndef CORTADO_COMMON_WIN32_ASYNC_STACK_TLS_H
#define CORTADO_COMMON_WIN32_ASYNC_STACK_TLS_H

#ifdef _WIN32

// Win32
//
#include <processthreadsapi.h>

namespace Cortado::Common
{

/// @brief Win32-specifc TLS index allocator.
///
struct Win32AsyncStackTLS
{
    /// @brief Get the current async stack frame pointer for this thread.
    /// @returns The stored pointer, or nullptr.
    ///
    static void *Get()
    {
        return TlsGetValue2(TlsIndex());
    }

    /// @brief Set the current async stack frame pointer for this thread.
    /// @param p The pointer to store.
    ///
    static void Set(void *ptr)
    {
        TlsSetValue(TlsIndex(), ptr);
    }

    /// @brief Get or allocate TLS index in Win32.
    /// @returns Allocated index.
    ///
    static DWORD TlsIndex()
    {
        static thread_local DWORD m_tlsIndex = TLS_OUT_OF_INDEXES;
        if (m_tlsIndex == TLS_OUT_OF_INDEXES)
        {
            m_tlsIndex = TlsAlloc();
        }

        return m_tlsIndex;
    }
};

} // namespace Cortado::Common

#endif // _WIN32

#endif // CORTADO_COMMON_WIN32_ASYNC_STACK_TLS_H
