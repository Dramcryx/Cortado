/// @file MacOSFutexLikeAtomic.h
/// FutexLikeAtomic implementation for macOS using os_sync_wait_on_address.
///

#ifndef CORTADO_COMMON_MACOS_FUTEX_LIKE_ATOMIC_H
#define CORTADO_COMMON_MACOS_FUTEX_LIKE_ATOMIC_H

#ifdef __APPLE__

// STL
//
#include <atomic>
#include <cerrno>

// macOS
//
#include <mach/mach_time.h>
#include <os/clock.h>
#include <os/os_sync_wait_on_address.h>

namespace Cortado::Common
{

/// @brief FutexLikeAtomic implementation for macOS.
/// Uses os_sync_wait_on_address / os_sync_wake_by_address APIs
/// available since macOS 14.4.
///
class MacOSFutexLikeAtomic : public std::atomic_int64_t
{
public:
    using std::atomic_int64_t::atomic_int64_t;

    /// @brief Wake one thread waiting on this address.
    ///
    void notify_one() noexcept
    {
        os_sync_wake_by_address_any(this,
                                    sizeof(*this),
                                    OS_SYNC_WAKE_BY_ADDRESS_NONE);
    }

    /// @brief Wake one thread waiting on this address (volatile overload).
    ///
    void notify_one() volatile noexcept
    {
        os_sync_wake_by_address_any((void*)this,
                                    sizeof(*this),
                                    OS_SYNC_WAKE_BY_ADDRESS_NONE);
    }

    /// @brief Wake all threads waiting on this address.
    ///
    void notify_all() noexcept
    {
        os_sync_wake_by_address_all(this,
                                    sizeof(*this),
                                    OS_SYNC_WAKE_BY_ADDRESS_NONE);
    }

    /// @brief Wake all threads waiting on this address (volatile overload).
    ///
    void notify_all() volatile noexcept
    {
        os_sync_wake_by_address_all((void*)this,
                                    sizeof(*this),
                                    OS_SYNC_WAKE_BY_ADDRESS_NONE);
    }

    /// @brief Block until value at this address differs from @p old.
    /// @param old Expected current value.
    ///
    void wait(std::int64_t old) const noexcept
    {
        os_sync_wait_on_address((void*)this,
                                old,
                                sizeof(old),
                                OS_SYNC_WAIT_ON_ADDRESS_NONE);
    }

    /// @brief Block until value at this address differs from @p old
    /// (volatile overload).
    /// @param old Expected current value.
    ///
    void wait(std::int64_t old) const volatile noexcept
    {
        os_sync_wait_on_address((void*)this,
                                old,
                                sizeof(old),
                                OS_SYNC_WAIT_ON_ADDRESS_NONE);
    }

    /// @brief Block until value differs from @p old or timeout expires.
    /// @param old Expected current value.
    /// @param timeoutMs Maximum time to wait in milliseconds.
    /// @returns true if woken normally, false on timeout.
    ///
    bool wait_for(std::int64_t old, std::uint32_t timeoutMs) const noexcept
    {
        uint64_t timeout = static_cast<uint64_t>(timeoutMs) * 1'000'000ULL;

        int result = os_sync_wait_on_address_with_timeout(
            (void*)this,
            old,
            sizeof(old),
            OS_SYNC_WAIT_ON_ADDRESS_NONE,
            OS_CLOCK_MACH_ABSOLUTE_TIME,
            timeout);

        return result == 0 || errno != ETIMEDOUT;
    }

    /// @brief Block until value differs from @p old or timeout expires
    /// (volatile overload).
    /// @param old Expected current value.
    /// @param timeoutMs Maximum time to wait in milliseconds.
    /// @returns true if woken normally, false on timeout.
    ///
    bool wait_for(std::int64_t old, std::uint32_t timeoutMs) const volatile noexcept
    {
        uint64_t timeout = static_cast<uint64_t>(timeoutMs) * 1'000'000ULL;

        int result = os_sync_wait_on_address_with_timeout(
            (void*)this,
            old,
            sizeof(old),
            OS_SYNC_WAIT_ON_ADDRESS_NONE,
            OS_CLOCK_MACH_ABSOLUTE_TIME,
            timeout);

        return result == 0 || errno != ETIMEDOUT;
    }
};

} // namespace Cortado::Common

#endif // __APPLE__

#endif // CORTADO_COMMON_MACOS_FUTEX_LIKE_ATOMIC_H
