/// @file ExampleCustomScheduler.cpp
/// Example of customizing an allocator.
///

// examples
//
#include "DefaultThreadId.h"

// Cortado
//
#include <Cortado/Await.h>

// STL
//
#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

class CustomScheduler
{
public:
    CustomScheduler() : m_workerThread{[this]{WorkerThreadFunc();}}
    {
    }

    ~CustomScheduler()
    {
        m_stop = true;
        m_workerThreadCv.notify_one();
        m_workerThread.join();
    }

    static CustomScheduler& GetDefaultBackgroundScheduler()
    {
        static CustomScheduler sched;
        return sched;
    }

    void Schedule(std::coroutine_handle<> h)
    {
        std::lock_guard lk{m_workerThreadMutex};
        m_coroQueue.push_back(h);
        m_workerThreadCv.notify_one();
    }

private:
    std::atomic_bool m_stop{false};
    std::mutex m_workerThreadMutex;
    std::condition_variable m_workerThreadCv;
    std::list<std::coroutine_handle<>> m_coroQueue;
    std::thread m_workerThread;

    void WorkerThreadFunc()
    {
        for (; !m_stop;)
        {
            std::coroutine_handle<> runNext{nullptr};
            {
                std::unique_lock lk{m_workerThreadMutex};

                if (m_stop)
                {
                    return;
                }

                if (m_coroQueue.empty())
                {
                    m_workerThreadCv.wait(lk);
                    if (m_stop)
                    {
                        return;
                    }
                }

                runNext = m_coroQueue.front();
                m_coroQueue.pop_front();
            }
            if (runNext == nullptr)
            {
                continue;
            }

            runNext();
        }
    }
};

struct TaskImplWithCustomScheduler :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    CustomScheduler
{
    using Event = Cortado::DefaultEvent;
};

template <typename T = void>
using Task = Cortado::Task<T, TaskImplWithCustomScheduler>;

Task<> RunAsync()
{
    int threadIdBefore = THREAD_ID;
    co_await Cortado::ResumeBackground();
    int threadIdAfter = THREAD_ID;

    std::cout << '[' << __FUNCTION__ << "] Thread ID before offloading " << threadIdBefore
              << " and after " << threadIdAfter << "\n";
}

Task<> RunAsync2()
{
    int threadIdBefore = THREAD_ID;
    co_await RunAsync();
    int threadIdAfter = THREAD_ID;

    std::cout << '[' << __FUNCTION__ << "] Thread ID before offloading " << threadIdBefore
              << " and after " << threadIdAfter << "\n";

    // Reschedule on the same scheduler, expect thread id remain the same
    //
    using Cortado::operator co_await;
    threadIdBefore = threadIdAfter;
    co_await CustomScheduler::GetDefaultBackgroundScheduler();
    threadIdAfter = THREAD_ID;

    std::cout << '[' << __FUNCTION__ << "] Thread ID after offloading " << threadIdBefore
              << " and after rescheduling on the same scheduler " << threadIdAfter << "\n";
}

int main()
{
    RunAsync2().Get();
    return 0;
}
