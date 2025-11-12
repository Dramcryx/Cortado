/// @file PosixEvent.h
/// Implementation of an event using POSIX condition variable.
///

#ifndef CORTADO_COMMON_POSIX_EVENT_H
#define CORTADO_COMMON_POSIX_EVENT_H

#if defined(_POSIX_VERSION) || defined(__APPLE__)

// POSIX
//
#include <pthread.h>

// STL
//
#include <atomic>

namespace Cortado::Common
{

/// @brief Event implementation for POSIX-like platform.
///
class PosixEvent
{
public:
    /// @brief Constructor. Initializes a non-singaled event.
    ///
    PosixEvent() :
        m_signaled{false}, m_mutex PTHREAD_MUTEX_INITIALIZER,
        m_cv PTHREAD_COND_INITIALIZER
    {
    }

    /// @brief Non-copyable.
    ///
    PosixEvent(const PosixEvent &) = delete;

    /// @brief Non-copyable.
    ///
    PosixEvent &operator=(const PosixEvent &) = delete;

    /// @brief Non-movable.
    ///
    PosixEvent(PosixEvent &&) = delete;

    /// @brief Non-movable.
    ///
    PosixEvent &operator=(PosixEvent &&) = delete;

    /// @brief Destructor.
    ///
    ~PosixEvent()
    {
        pthread_cond_destroy(&m_cv);
        pthread_mutex_destroy(&m_mutex);
    }

    /// @brief Concept contract: Wait for event to be set.
    ///
    void Wait()
    {
        PthreadAutoLock autoLock{&m_mutex};

        while (!m_signaled.load())
        {
            pthread_cond_wait(&m_cv, &m_mutex);
        }
    }

    /// @brief Concept contract: Wait for event to be set in a period of time.
    /// @param timeToWaitMs How many milliseconds to wait.
    /// @returns true if event was set in timeToWaitMs, false otherwise.
    ///
    bool WaitFor(unsigned long timeToWaitMs)
    {
        int result = 0;
        for (; !m_signaled && result == 0;)
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            ts.tv_nsec += timeToWaitMs * 1000000;

            if (ts.tv_nsec >= 1000000000)
            {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec += 1;
            }

            PthreadAutoLock autoLock{&m_mutex};

            result = pthread_cond_timedwait(&m_cv, &m_mutex, &ts);
        }

        return m_signaled;
    }

    /// @brief Concept contact: Singal the event.
    ///
    void Set()
    {
        PthreadAutoLock autoLock{&m_mutex};

        m_signaled.store(true);
        pthread_cond_broadcast(&m_cv);
    }

    /// @brief Concept contract: Test if event is set already.
    /// @return true if event is set, false otherwise.
    ///
    bool IsSet()
    {
        PthreadAutoLock autoLock{&m_mutex};
        return m_signaled;
    }

private:
    std::atomic_bool m_signaled;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cv;

    struct PthreadAutoLock
    {
        pthread_mutex_t *m_mutex;

        PthreadAutoLock(pthread_mutex_t *mutex) : m_mutex{mutex}
        {
            pthread_mutex_lock(m_mutex);
        }

        ~PthreadAutoLock()
        {
            pthread_mutex_unlock(m_mutex);
        }
    };
};

} // namespace Cortado::Common

#endif // _POSIX_VERSION || __APPLE__

#endif
