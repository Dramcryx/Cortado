/// @file LinuxFutexLikeAtomic.h
/// FutexLikeAtomic implementation for Linux using raw futex syscalls.
///

#ifndef CORTADO_COMMON_LINUX_FUTEX_LIKE_ATOMIC_H
#define CORTADO_COMMON_LINUX_FUTEX_LIKE_ATOMIC_H

#ifdef __linux__

// STL
//
#include <atomic>
#include <climits>
#include <cstdint>

// Linux
//
#include <cerrno>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace Cortado::Common
{

/// @brief FutexLikeAtomic implementation for Linux.
/// Wraps raw futex syscalls to provide wait/notify semantics
/// on top of std::atomic_int64_t.
///
class LinuxFutexLikeAtomic : public std::atomic_int64_t
{
public:
    using std::atomic_int64_t::atomic_int64_t;

    /// @brief Wake one thread waiting on this address.
    ///
    void notify_one() noexcept
    {
        syscall(SYS_futex,
                reinterpret_cast<int *>(this),
                FUTEX_WAKE,
                1,
                nullptr,
                nullptr,
                0);
    }

    /// @brief Wake one thread waiting on this address (volatile overload).
    ///
    void notify_one() volatile noexcept
    {
        syscall(SYS_futex,
                reinterpret_cast<volatile int *>(this),
                FUTEX_WAKE,
                1,
                nullptr,
                nullptr,
                0);
    }

    /// @brief Wake all threads waiting on this address.
    ///
    void notify_all() noexcept
    {
        syscall(SYS_futex,
                reinterpret_cast<int *>(this),
                FUTEX_WAKE,
                INT_MAX,
                nullptr,
                nullptr,
                0);
    }

    /// @brief Wake all threads waiting on this address (volatile overload).
    ///
    void notify_all() volatile noexcept
    {
        syscall(SYS_futex,
                reinterpret_cast<volatile int *>(this),
                FUTEX_WAKE,
                INT_MAX,
                nullptr,
                nullptr,
                0);
    }

    /// @brief Block until value at this address differs from @p old.
    /// @param old Expected current value.
    ///
    void wait(std::int64_t old) const noexcept
    {
        syscall(SYS_futex,
                reinterpret_cast<const int *>(this),
                FUTEX_WAIT,
                old,
                nullptr,
                nullptr,
                0);
    }

    /// @brief Block until value at this address differs from @p old
    /// (volatile overload).
    /// @param old Expected current value.
    ///
    void wait(std::int64_t old) const volatile noexcept
    {
        syscall(SYS_futex,
                reinterpret_cast<const volatile int *>(this),
                FUTEX_WAIT,
                old,
                nullptr,
                nullptr,
                0);
    }

    /// @brief Block until value differs from @p old or timeout expires.
    /// @param old Expected current value.
    /// @param timeoutMs Maximum time to wait in milliseconds.
    /// @returns true if woken normally, false on timeout.
    ///
    bool wait_for(std::int64_t old, std::uint32_t timeoutMs) const noexcept
    {
        std::uint64_t timeoutNs = static_cast<uint64_t>(timeoutMs) * 1'000'000ULL;

        struct timespec ts;
        ts.tv_sec = timeoutNs / 1'000'000'000ULL;
        ts.tv_nsec = timeoutNs % 1'000'000'000ULL;

        int r = syscall(SYS_futex,
                        reinterpret_cast<const int *>(this),
                        FUTEX_WAIT,
                        old,
                        &ts,
                        nullptr,
                        0);

        return r == 0 || errno != ETIMEDOUT;
    }

    /// @brief Block until value differs from @p old or timeout expires
    /// (volatile overload).
    /// @param old Expected current value.
    /// @param timeoutMs Maximum time to wait in milliseconds.
    /// @returns true if woken normally, false on timeout.
    ///
    bool wait_for(std::int64_t old, std::uint32_t timeoutMs) const volatile noexcept
    {
        std::uint64_t timeoutNs = static_cast<uint64_t>(timeoutMs) * 1'000'000ULL;

        struct timespec ts;
        ts.tv_sec = timeoutNs / 1'000'000'000ULL;
        ts.tv_nsec = timeoutNs % 1'000'000'000ULL;

        int r = syscall(SYS_futex,
                        reinterpret_cast<const volatile int *>(this),
                        FUTEX_WAIT,
                        old,
                        &ts,
                        nullptr,
                        0);

        return r == 0 || errno != ETIMEDOUT;
    }
};

} // namespace Cortado::Common

#endif // __linux__

#endif // CORTADO_COMMON_LINUX_FUTEX_LIKE_ATOMIC_H
