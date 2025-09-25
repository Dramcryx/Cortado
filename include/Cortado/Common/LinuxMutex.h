/// @file LinuxMutex.h
/// Futex-based implementation of mutex.
///

// STL
//
#include <atomic>
#include <cerrno>

// Linux
//
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef CORTADO_COMMON_LINUX_MUTEX_H
#define CORTADO_COMMON_LINUX_MUTEX_H

namespace Cortado::Common
{

/// @brief Implementation using futex.
///
class LinuxMutex
{
public:
    /// @brief Constructor.
    ///
    LinuxMutex() = default;

    /// @brief Non-copyable.
    ///
    LinuxMutex(const LinuxMutex &) = delete;

    /// @brief Non-copyable.
    ///
    LinuxMutex &operator=(const LinuxMutex &) = delete;

    /// @brief Non-movable.
    ///
    LinuxMutex(LinuxMutex &&) = delete;

    /// @brief Non-movable.
    ///
    LinuxMutex &operator=(LinuxMutex &&) = delete;

    /// @brief Destructor.
    ///
    ~LinuxMutex() = default;

    /// @brief Concept contract: Lock mutex, waiting if needed.
    ///
    void lock() noexcept
    {
        unsigned long expected = 0;
        // Fast path: try to acquire
        if (m_state.compare_exchange_strong(expected,
                                            1,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed))
        {
            return;
        }

        // Slow path: contention
        while (true)
        {
            expected = 0;
            if (m_state.compare_exchange_strong(expected,
                                                1,
                                                std::memory_order_acquire,
                                                std::memory_order_relaxed))
            {
                return; // acquired
            }
            futex_wait();
        }
    }

    /// @brief Concept contract: Try locking mutex without waiting.
    ///
    bool try_lock() noexcept
    {
        unsigned long expected = 0;
        return m_state.compare_exchange_strong(expected,
                                               1,
                                               std::memory_order_acquire,
                                               std::memory_order_relaxed);
    }

    /// @brief Concept contract: Unlock mutex and wake one waiter if any.
    ///
    void unlock() noexcept
    {
        m_state.store(0, std::memory_order_release);
        futex_wake();
    }

private:
    std::atomic_ulong m_state{0};

    void futex_wait() noexcept
    {
        int one = 1;
        // Wait while *m_state == 1
        int res = syscall(SYS_futex,
                          reinterpret_cast<int *>(&m_state),
                          FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
                          one,
                          nullptr,
                          nullptr,
                          0);

        if (res == -1 && errno != EAGAIN && errno != EINTR)
        {
            // real code would handle errors properly
        }
    }

    void futex_wake() noexcept
    {
        syscall(SYS_futex,
                reinterpret_cast<int *>(&m_state),
                FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
                1,
                nullptr,
                nullptr,
                0);
    }
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_LINUX_MUTEX_H
