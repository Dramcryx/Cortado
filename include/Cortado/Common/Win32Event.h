/// @file Win32Event.h
/// Implementation of an event using Win32 API.
///

#ifndef CORTADO_COMMON_WIN32_EVENT_H
#define CORTADO_COMMON_WIN32_EVENT_H

#ifdef _WIN32

// Win32
//
#include <windows.h>

namespace Cortado::Common
{

/// @brief Event implementation for Win32 platform.
///
class Win32Event
{
public:
    /// @brief Constructor. Initializes a non-singaled event.
    ///
    Win32Event() : m_eventHandle{::CreateEvent(nullptr, true, false, nullptr)}
    {
    }

    /// @brief Non-copyable.
    ///
    Win32Event(const Win32Event &) = delete;

    /// @brief Non-copyable.
    ///
    Win32Event &operator=(const Win32Event &) = delete;

    /// @brief Non-movable.
    ///
    Win32Event(Win32Event &&) = delete;

    /// @brief Non-movable.
    ///
    Win32Event &operator=(Win32Event &&) = delete;

    /// @brief Destructor.
    ///
    ~Win32Event()
    {
        ::CloseHandle(m_eventHandle);
    }

    /// @brief Concept contract: Wait for event to be set.
    ///
    void Wait()
    {
        ::WaitForSingleObject(m_eventHandle, INFINITE);
    }

    /// @brief Concept contract: Wait for event to be set in a period of time.
    /// @param timeToWaitMs How many milliseconds to wait.
    /// @returns true if event was set in timeToWaitMs, false otherwise.
    ///
    bool WaitFor(unsigned long timeToWaitMs)
    {
        return ::WaitForSingleObject(m_eventHandle, timeToWaitMs) ==
               WAIT_OBJECT_0;
    }

    /// @brief Concept contact: Singal the event.
    ///
    void Set()
    {
        SetEvent(m_eventHandle);
    }

    /// @brief Concept contract: Test if event is set already.
    /// @return true if event is set, false otherwise.
    ///
    bool IsSet()
    {
        return WaitFor(0);
    }

private:
    HANDLE m_eventHandle;
};

} // namespace Cortado::Common

#endif // _WIN32

#endif
