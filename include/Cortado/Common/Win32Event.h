#ifndef CORTADO_COMMON_WIN32_EVENT_H
#define CORTADO_COMMON_WIN32_EVENT_H

// Win32
//
#include <windows.h>

namespace Cortado::Common
{

class Win32Event
{
public:
    Win32Event() :
        m_eventHandle{::CreateEvent(nullptr, true, false, nullptr)}
    {
    }

    ~Win32Event()
    {
        ::CloseHandle(m_eventHandle);
    }

    
    void Wait()
    {
        ::WaitForSingleObject(m_eventHandle, INFINITE);
    }

    bool WaitFor(unsigned long timeToWaitMs)
    {
        return ::WaitForSingleObject(m_eventHandle, timeToWaitMs) == WAIT_OBJECT_0;
    }

    void Set()
    {
        SetEvent(m_eventHandle);
    }

    bool IsSet()
    {
        return WaitFor(0);
    }

private:
    HANDLE m_eventHandle;
};

} // namespace Cortado::Common


#endif
