/// @file MacOSMutex.h
/// Implementation of faster mutex for macOS.
///

#ifndef CORTADO_COMMON_MACOS_MUTEX_H
#define CORTADO_COMMON_MACOS_MUTEX_H

#ifdef __APPLE__

// macOS
//
#include <AvailabilityMacros.h>
#include <os/lock.h>
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 140400 // Since macOS 14.4
#define MACOS_HAS_WAIT_ON_ADDRESS
#include <os/os_sync_wait_on_address.h>
#endif // __MAC_OS_X_VERSION_MAX_ALLOWED >= 140400

// STL
//
#include <atomic>

namespace Cortado::Common
{

namespace V1
{

/// @brief Wider supported implementation - uses os_unfair_lock.
///
class MacOSMutex
{
public:
    /// @brief Constructor.
    ///
    MacOSMutex() = default;

    /// @brief Non-copyable.
    ///
    MacOSMutex(const MacOSMutex &) = delete;

    /// @brief Non-copyable.
    ///
    MacOSMutex &operator=(const MacOSMutex &) = delete;

    /// @brief Non-movable.
    ///
    MacOSMutex(MacOSMutex &&) = delete;

    /// @brief Non-movable.
    ///
    MacOSMutex &operator=(MacOSMutex &&) = delete;

    /// @brief Destructor.
    ///
    ~MacOSMutex() = default;

	/// @brief Concept contract: Call macOS API to lock the mutex.
    ///
    void lock() noexcept
    {
        os_unfair_lock_lock(&m_lock);
    }

	/// @brief Concept contract:
    /// Call macOS API to try locking the mutex without blocking.
    ///
    bool try_lock() noexcept
    {
        return os_unfair_lock_trylock(&m_lock);
    }

	/// @brief Concept contract: Call macOS API to unlock the mutex.
    ///
    void unlock() noexcept
    {
        os_unfair_lock_unlock(&m_lock);
    }

private:
    os_unfair_lock m_lock{OS_UNFAIR_LOCK_INIT};
};

} // namespace V1

#ifdef MACOS_HAS_WAIT_ON_ADDRESS

namespace V2
{

/// @brief Implementation that uses os_sync_wake_by_address_any,
/// which in some case should be quicker.
///
class MacOSMutex
{
public:
    /// @brief Constructor.
    ///
    MacOSMutex() = default;

    /// @brief Non-copyable.
    ///
    MacOSMutex(const MacOSMutex &) = delete;

    /// @brief Non-copyable.
    ///
    MacOSMutex &operator=(const MacOSMutex &) = delete;

    /// @brief Non-movable.
    ///
    MacOSMutex(MacOSMutex &&) = delete;

    /// @brief Non-movable.
    ///
    MacOSMutex &operator=(MacOSMutex &&) = delete;

    /// @brief Destructor.
    ///
    ~MacOSMutex() = default;

	/// @brief Concept contract: Lock mutex, waiting if needed.
    ///
    void lock() noexcept
    {
        unsigned long long expected = 0;
        // fast path: try without blocking
        if (m_state.compare_exchange_strong(expected,
                                            1,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed))
        {
            return;
        }

        // otherwise, contended — spin a little (optional), then wait
        while (true)
        {
            expected = 0;
            // if lock becomes free
            if (m_state.load(std::memory_order_relaxed) == 0)
            {
                if (m_state.compare_exchange_strong(expected,
                                                    1,
                                                    std::memory_order_acquire,
                                                    std::memory_order_relaxed))
                {
                    return;
                }
                // else something else acquired, continue
            }
            // Wait: block if state_ is 1
            os_sync_wait_on_address(&m_state,
                                    (uint64_t)1,
                                    sizeof(m_state),
                                    OS_SYNC_WAIT_ON_ADDRESS_NONE);
            // After waking, loop to try again
        }
    }

	/// @brief Concept contract: Try locking mutex without waiting.
    ///
    bool try_lock() noexcept
    {
        uint64_t expected = 0;
        return m_state.compare_exchange_strong(expected,
                                               1,
                                               std::memory_order_acquire,
                                               std::memory_order_relaxed);
    }

	/// @brief Concept contract: Unlock mutex, waking one waiter if any.
    ///
    void unlock() noexcept
    {
        // store zero
        m_state.store(0, std::memory_order_release);
        // wake one waiting thread
        os_sync_wake_by_address_any(&m_state,
                                    sizeof(m_state),
                                    OS_SYNC_WAKE_BY_ADDRESS_NONE);
    }

private:
    std::atomic<uint64_t> m_state;
};

} // namespace V2

using MacOSMutex = V2::MacOSMutex;
#else
using MacOSMutex = V1::MacOSMutex;
#endif // MACOS_HAS_WAIT_ON_ADDRESS

} // namespace Cortado::Common

#endif // __APPLE__

#endif // CORTADO_COMMON_MACOS_MUTEX_H
