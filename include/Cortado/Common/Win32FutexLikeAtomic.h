/// @file Win32FutexLikeAtomic.h
/// FutexLikeAtomic implementation for Win32 using WaitOnAddress /
/// WakeByAddress APIs.
///

#ifndef CORTADO_COMMON_WIN32_FUTEX_LIKE_ATOMIC_H
#define CORTADO_COMMON_WIN32_FUTEX_LIKE_ATOMIC_H

#ifdef _WIN32

// STL
//
#include <atomic>

// Win32
//
#include <synchapi.h>
#include <windows.h>

namespace Cortado::Common
{

/// @brief FutexLikeAtomic implementation for Win32.
/// Uses WaitOnAddress / WakeByAddress* APIs available since
/// Windows 8 / Windows Server 2012.
///
class Win32FutexLikeAtomic : public std::atomic_int64_t
{
public:
    using std::atomic_int64_t::atomic_int64_t;

    /// @brief Wake one thread waiting on this address.
    ///
    void notify_one() noexcept
    {
        ::WakeByAddressSingle(this);
    }

    /// @brief Wake one thread waiting on this address (volatile overload).
    ///
    void notify_one() volatile noexcept
    {
        ::WakeByAddressSingle((void *)this);
    }

    /// @brief Wake all threads waiting on this address.
    ///
    void notify_all() noexcept
    {
        ::WakeByAddressAll(this);
    }

    /// @brief Wake all threads waiting on this address (volatile overload).
    ///
    void notify_all() volatile noexcept
    {
        ::WakeByAddressAll((void*)this);
    }

    /// @brief Block until value at this address differs from @p old.
    /// @param old Expected current value.
    ///
    void wait(std::int64_t old) const noexcept
    {
        ::WaitOnAddress((void*)this, &old, sizeof(Win32FutexLikeAtomic), INFINITE);
    }

    /// @brief Block until value at this address differs from @p old
    /// (volatile overload).
    /// @param old Expected current value.
    ///
    void wait(std::int64_t old) const volatile noexcept
    {
        ::WaitOnAddress((void*)this, &old, sizeof(Win32FutexLikeAtomic), INFINITE);
    }

    /// @brief Block until value differs from @p old or timeout expires.
    /// @param old Expected current value.
    /// @param timeoutMs Maximum time to wait in milliseconds.
    /// @returns true if woken normally, false on timeout.
    ///
    bool wait_for(std::int64_t old, std::uint32_t timeoutMs) const noexcept
    {
        auto ok = ::WaitOnAddress((void *)this,
                                  &old,
                                  sizeof(Win32FutexLikeAtomic),
                                  timeoutMs);

        return ok != 0 || GetLastError() == ERROR_SUCCESS;
    }

    /// @brief Block until value differs from @p old or timeout expires
    /// (volatile overload).
    /// @param old Expected current value.
    /// @param timeoutMs Maximum time to wait in milliseconds.
    /// @returns true if woken normally, false on timeout.
    ///
    bool wait_for(std::int64_t old, std::uint32_t timeoutMs) const volatile noexcept
    {
        auto ok = ::WaitOnAddress((void *)this,
                                  &old,
                                  sizeof(Win32FutexLikeAtomic),
                                  timeoutMs);

        return ok != 0 || GetLastError() == ERROR_SUCCESS;
    }
};

} // namespace Cortado::Common

#endif // _WIN32

#endif
