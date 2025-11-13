/// @file MacOSMutex.h
/// Implementation of faster mutex for macOS.
///

#ifndef CORTADO_COMMON_MACOS_MUTEX_H
#define CORTADO_COMMON_MACOS_MUTEX_H

#include <cstdint>
#ifdef __APPLE__

// Cortado
//
#include <Cortado/Common/MutexBase.h>

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
class MacOSMutex : public MutexBase<MacOSMutex>
{
public:
    /// @brief MutexBase contract: wait futex
    /// @param state Futex to wait
    ///
    void WaitOnAddress(std::atomic<int>* state)
    {
        uint64_t expected = 1;
        os_sync_wait_on_address(state, expected, sizeof(expected),
                                OS_SYNC_WAIT_ON_ADDRESS_NONE);
    }

    /// @brief MutexBase contract: wake futex waiter
    /// @param state Futex to wake
    ///
    void WakeOne(std::atomic<int>* state)
    {
        os_sync_wake_by_address_any(state,
                                    sizeof(*state),
                                    OS_SYNC_WAKE_BY_ADDRESS_NONE);
    }
};

} // namespace V2

using MacOSMutex = V2::MacOSMutex;
#else
using MacOSMutex = V1::MacOSMutex;
#endif // MACOS_HAS_WAIT_ON_ADDRESS

} // namespace Cortado::Common

#endif // __APPLE__

#endif // CORTADO_COMMON_MACOS_MUTEX_H
