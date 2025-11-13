/// @file LinuxMutex.h
/// Futex-based implementation of mutex.
///

#ifndef CORTADO_COMMON_LINUX_MUTEX_H
#define CORTADO_COMMON_LINUX_MUTEX_H

// Cortado
//
#include <Cortado/Common/MutexBase.h>

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
class LinuxMutex : public MutexBase<LinuxMutex>
{
public:
    void WaitOnAddress(std::atomic<int> *state)
    {
        int expected = 1;
        syscall(SYS_futex,
                reinterpret_cast<int *>(state),
                FUTEX_WAIT,
                expected,
                nullptr,
                nullptr,
                0);
    }

    void WakeOne(std::atomic<int> *state)
    {
        syscall(SYS_futex,
                reinterpret_cast<int *>(state),
                FUTEX_WAKE,
                1,
                nullptr,
                nullptr,
                0);
    }
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_LINUX_MUTEX_H
