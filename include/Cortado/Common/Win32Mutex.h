/// @file Win32Mutex.h
/// Implementation of mutex for Win32.
///

#ifndef CORTADO_COMMON_WIN32_MUTEX_H
#define CORTADO_COMMON_WIN32_MUTEX_H

#ifdef _WIN32

// Win32
//
#include <synchapi.h>

// STL
//
#include <atomic>

namespace Cortado::Common
{

/// @brief Implementation using Win32 WaitOnAddress.
///
class Win32Mutex
{
public:
    /// @brief Constructor.
    ///
    Win32Mutex() = default;

    /// @brief Non-copyable.
    ///
    Win32Mutex(const Win32Mutex &) = delete;

    /// @brief Non-copyable.
    ///
    Win32Mutex &operator=(const Win32Mutex &) = delete;

    /// @brief Non-movable.
    ///
    Win32Mutex(Win32Mutex &&) = delete;

    /// @brief Non-movable.
    ///
    Win32Mutex &operator=(Win32Mutex &&) = delete;

    /// @brief Destructor.
    ///
    ~Win32Mutex() = default;

    /// @brief Concept contract: Lock mutex, waiting if needed.
    ///
    void lock() noexcept
    {
        unsigned long expected = 0;
        while (!m_state.compare_exchange_weak(expected,
                                              1,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed))
        {
            expected = 0;
            // Wait until state_ changes
            WaitOnAddress(&m_state, &expected, sizeof(expected), INFINITE);
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
        WakeByAddressSingle(&m_state);
    }

private:
    std::atomic_ulong m_state{0};
};

} // namespace Cortado::Common

#endif // _WIN32
#endif // CORTADO_COMMON_WIN32_MUTEX_H
