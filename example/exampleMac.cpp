// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Common/STLAtomicIncDec.h>
#include <Cortado/Common/STLCoroutineAllocator.h>
#include <Cortado/Common/STLExceptionHandler.h>
#include <Cortado/Common/MacOSCoroutineScheduler.h>
#include <Cortado/Common/MacOSAtomicCompareExchange.h>

// macOS
//
#include <pthread.h>

// STL
//
#include <iostream>


struct STLWithMacOSTaskImpl :
    Cortado::Common::STLAtomicIncDec,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    Cortado::Common::MacOSCoroutineScheduler,
    Cortado::Common::MacOSAtomicCompareExchange
{
    inline static void YieldCurrentThread()
    {
        pthread_yield_np();
    }
};

template <typename R = void>
using Task = Cortado::Task<STLWithMacOSTaskImpl, R>;

Task<int> Ans()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    co_await Cortado::ResumeBackground();
    std::cout << __FUNCTION__ << " Resumed on thread " << pthread_self() << "\n";
    co_return 42;
}

Task<> Ans2()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    Task<int> tasks[]{ Ans(), Ans(), Ans() };
    co_await Cortado::WhenAll(tasks[0], tasks[1], tasks[2]);
    std::cout << __FUNCTION__ << " Resumed on thread " << pthread_self() << "\n";
    co_return;
}

int main()
{
    Ans2().Get();
    return 0;
}