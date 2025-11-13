/// @file Win32Event.h
/// Implementation of an event using Win32 API.
///

#ifndef CORTADO_COMMON_WIN32_EVENT_H
#define CORTADO_COMMON_WIN32_EVENT_H

#ifdef _WIN32

// Cortado
//
#include <Cortado/Common/EventBase.h>

// Win32
//
#include <synchapi.h>
#include <windows.h>

namespace Cortado::Common
{

/// @brief Event implementation for Win32 platform.
///
class Win32Event : public EventBase<Win32Event>
{
public:
    void WakeAll(std::atomic<int>* state)
    {
        WakeByAddressAll(state);
    }

    bool WaitUntil(std::atomic<int>* state, uint64_t timeoutNs)
    {
        int expected = 0;

        DWORD timeoutMs = (timeoutNs == UINT64_MAX)
                         ? INFINITE
                         : static_cast<DWORD>(timeoutMs / 1'000'000ULL);

        BOOL ok = WaitOnAddress(state, &expected, sizeof(expected), timeoutMs);

        return ok != 0 || GetLastError() == ERROR_SUCCESS;
    }
};

} // namespace Cortado::Common

#endif // _WIN32

#endif
