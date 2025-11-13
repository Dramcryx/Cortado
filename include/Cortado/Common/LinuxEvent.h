/// @file LinuxEvent.h
/// Implementation of event for Linux.
///

#ifndef CORTADO_COMMON_LINUX_EVENT_H
#define CORTADO_COMMON_LINUX_EVENT_H

// Cortado
//
#include <Cortado/Common/EventBase.h>

// Linux
//
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

// STL
//
#include <cerrno>

namespace Cortado::Common
{

/// @brief Implementation using futex.
///
class LinuxEvent : public EventBase<LinuxEvent>
{
public:
    /// @brief EventBase contract: wake all futex waiters.
    /// @param state Futex address
    ///
    void WakeAll(std::atomic<int> *state)
    {
        syscall(SYS_futex,
                reinterpret_cast<int *>(state),
                FUTEX_WAKE,
                INT_MAX,
                nullptr,
                nullptr,
                0);
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
            syscall(SYS_futex,
                    reinterpret_cast<int *>(state),
                    FUTEX_WAIT,
                    expected,
                    nullptr,
                    nullptr,
                    0);

            return true;
        }
        else
        {
            struct timespec ts;
            ts.tv_sec = timeoutNs / 1'000'000'000ULL;
            ts.tv_nsec = timeoutNs % 1'000'000'000ULL;

            int r = syscall(SYS_futex,
                            reinterpret_cast<int *>(state),
                            FUTEX_WAIT,
                            expected,
                            &ts,
                            nullptr,
                            0);

            return r == 0 || errno != ETIMEDOUT;
        }

        return EventBase<LinuxEvent>::IsSet();
    }
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_LINUX_EVENT_H
