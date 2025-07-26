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
    PosixEvent(bool initiallySignaled = false) :
        m_signaled{initiallySignaled}
    {
        pthread_mutex_init(&m_mutex, nullptr);
        pthread_cond_init(&m_cv, nullptr);
        m_signaled = false;
    }

    PosixEvent(const PosixEvent&) = delete;
    PosixEvent& operator=(const PosixEvent&) = delete;

    PosixEvent(PosixEvent&&) = delete;
    PosixEvent& operator=(PosixEvent&&) = delete;

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

    void Signal()
    {
        PthreadAutoLock autoLock{&m_mutex};

        m_signaled = true;
        pthread_cond_broadcast(&m_cv);
    }

private:
    bool m_signaled = false;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cv;

    struct PthreadAutoLock
    {
        pthread_mutex_t* m_mutex;

        PthreadAutoLock(pthread_mutex_t* mutex) :
            m_mutex{mutex}
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
