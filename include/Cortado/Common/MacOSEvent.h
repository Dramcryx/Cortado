/// @file MacOSEvent.h
/// Implementation of event for macOS.
///

#ifndef CORTADO_COMMON_MACOS_EVENT_H
#define CORTADO_COMMON_MACOS_EVENT_H

#ifdef __APPLE__

// Cortado
//
#include <Cortado/Common/EventBase.h>

// macOS
//
#include <os/os_sync_wait_on_address.h>

namespace Cortado::Common
{

/// @brief Implementation using macOS os_sync_wake_by_address_any.
///
class MacOSEvent : public EventBase<MacOSEvent>
{
public:
    /// @brief EventBase contract: wake all futex waiters.
    /// @param state Futex address
    ///
    void WakeAll(std::atomic<int> *state)
    {
        os_sync_wake_by_address_all(state,
                                    sizeof(*state),
                                    OS_SYNC_WAKE_BY_ADDRESS_NONE);
    }

    /// @brief EventBase contract: wait futex with given timeout
    /// @param state Futex address
    /// @param timeoutNs Timeout to wait in nanoseconds
    /// @returns true if event was set, false otherwise
    ///
    bool WaitForImpl(std::atomic<int> *state, uint64_t timeoutNs)
    {
        int expected = 0;

        if (timeoutNs == UINT64_MAX)
        {
            os_sync_wait_on_address(state,
                                    expected,
                                    sizeof(expected),
                                    OS_SYNC_WAIT_ON_ADDRESS_NONE);
        }
        else
        {
            os_sync_wait_on_address_with_timeout(state,
                                                 expected,
                                                 sizeof(expected),
                                                 OS_SYNC_WAIT_ON_ADDRESS_NONE,
                                                 OS_CLOCK_MACH_ABSOLUTE_TIME,
                                                 timeoutNs);
        }

        return EventBase<MacOSEvent>::IsSet();
    }
};

} // namespace Cortado::Common

#endif // __APPLE__
#endif // CORTADO_COMMON_MACOS_EVENT_H
