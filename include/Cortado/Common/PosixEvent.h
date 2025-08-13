#ifndef CORTADO_COMMON_POSIX_EVENT_H
#define CORTADO_COMMON_POSIX_EVENT_H

// POSIX
//
#include <pthread.h>

namespace Cortado::Common
{

class PosixEvent
{
public:
    PosixEvent(bool initiallySignaled = false) : m_signaled{initiallySignaled}
    {
        m_mutex = PTHREAD_MUTEX_INITIALIZER;
        m_cv = PTHREAD_COND_INITIALIZER;
        m_signaled = false;
    }

    PosixEvent(const PosixEvent &) = delete;
    PosixEvent &operator=(const PosixEvent &) = delete;

    PosixEvent(PosixEvent &&) = delete;
    PosixEvent &operator=(PosixEvent &&) = delete;

    ~PosixEvent()
    {
        pthread_cond_destroy(&m_cv);
        pthread_mutex_destroy(&m_mutex);
    }

    void Wait()
    {
        PthreadAutoLock autoLock{&m_mutex};

        while (!m_signaled)
        {
            pthread_cond_wait(&m_cv, &m_mutex);
        }
    }

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

    void Set()
    {
        PthreadAutoLock autoLock{&m_mutex};

        m_signaled = true;
        pthread_cond_broadcast(&m_cv);
    }

    bool IsSet()
    {
        PthreadAutoLock autoLock{&m_mutex};
        return m_signaled;
    }

private:
    bool m_signaled = false;
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

#endif
