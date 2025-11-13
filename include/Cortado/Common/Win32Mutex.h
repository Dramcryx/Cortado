/// @file Win32Mutex.h
/// Implementation of mutex for Win32.
///

#ifndef CORTADO_COMMON_WIN32_MUTEX_H
#define CORTADO_COMMON_WIN32_MUTEX_H

#ifdef _WIN32

// Cortado
//
#include <Cortado/Common/MutexBase.h>

// Win32
//
#include <synchapi.h>
#include <windows.h>

namespace Cortado::Common
{

/// @brief Implementation using Win32 WaitOnAddress.
///
class Win32Mutex : public MutexBase<Win32Mutex>
{
public:
    void WaitOnAddress(std::atomic<int> *state)
    {
        int expected = 1;
        ::WaitOnAddress(state, &expected, sizeof(expected), INFINITE);
    }

    void WakeOne(std::atomic<int> *state)
    {
        ::WakeByAddressSingle(state);
    }
};

} // namespace Cortado::Common

#endif // _WIN32
#endif // CORTADO_COMMON_WIN32_MUTEX_H
