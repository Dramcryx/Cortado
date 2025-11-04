/// @file PosixCoroutineScheduler.h
/// Implementation of a default async runtime using pthread.
///

#ifndef CORTADO_COMMON_POSIX_COROUTINE_SCHEDULER_H
#define CORTADO_COMMON_POSIX_COROUTINE_SCHEDULER_H

// POSIX
//
#include <pthread.h>

// STL
//
#include <coroutine>
#include <queue>
#include <thread>
#include <vector>

namespace Cortado::Common
{

class PosixCoroutineScheduler
{
public:
    /// @brief Constructs a thread pool with
    /// numThreads threads.
    /// @param numThreads Number of threads in pool.
    ///
    PosixCoroutineScheduler(size_t numThreads = std::thread::hardware_concurrency()) :
        stop{false}
    {
        pthread_mutex_init(&m_queueMutex, nullptr);
        pthread_cond_init(&m_condition, nullptr);

        for (size_t i = 0; i < numThreads; ++i)
        {
            pthread_t thread;
            pthread_create(&thread, nullptr, WorkerFn, this);
            m_threads.push_back(thread);
        }
    }

    /// @brief Stops and destroys threadpool
    ///
    ~PosixCoroutineScheduler()
    {
        Shutdown();
        pthread_mutex_destroy(&m_queueMutex);
        pthread_cond_destroy(&m_condition);
    }

    /// @brief Concept contract: Schedules coroutine in a different thread.
    /// @param h Coroutine to schedule.
    ///
    void Schedule(std::coroutine_handle<> h)
    {
        pthread_mutex_lock(&m_queueMutex);
        m_tasks.push(std::move(h));
        pthread_cond_signal(&m_condition);
        pthread_mutex_unlock(&m_queueMutex);
    }

    /// @brief Concept contract: Get app-global scheduler instance.
    ///
    static PosixCoroutineScheduler &GetDefaultBackgroundScheduler()
    {
        static PosixCoroutineScheduler sched;
        return sched;
    }

private:
    /// @brief Worker thread callback for pthread
    /// @param arg Type-erased scheduler object
    ///
    static void *WorkerFn(void *arg)
    {
        PosixCoroutineScheduler *pool = static_cast<PosixCoroutineScheduler *>(arg);
        pool->Run();
        return nullptr;
    }

    /// @brief Worker thread entry point
    ///
    void Run()
    {
        while (true)
        {
            std::coroutine_handle<> task;

            pthread_mutex_lock(&m_queueMutex);
            while (m_tasks.empty() && !stop)
            {
                pthread_cond_wait(&m_condition, &m_queueMutex);
            }

            if (stop && m_tasks.empty())
            {
                pthread_mutex_unlock(&m_queueMutex);
                break;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
            pthread_mutex_unlock(&m_queueMutex);

            task();
        }
    }

    /// @brief Shuts down threads
    ///
    void Shutdown()
    {
        pthread_mutex_lock(&m_queueMutex);
        stop = true;
        pthread_cond_broadcast(&m_condition);
        pthread_mutex_unlock(&m_queueMutex);

        for (pthread_t t : m_threads)
        {
            pthread_join(t, nullptr);
        }

        m_threads.clear();
    }

    std::vector<pthread_t> m_threads;
    std::queue<std::coroutine_handle<>> m_tasks;
    pthread_mutex_t m_queueMutex;
    pthread_cond_t m_condition;
    bool stop;
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_POSIX_COROUTINE_SCHEDULER_H
