// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Common/STLAtomicIncDec.h>
#include <Cortado/Common/STLCoroutineAllocator.h>
#include <Cortado/Common/STLExceptionHandler.h>
#include <Cortado/Common/Win32CoroutineScheduler.h>
#include <Cortado/Common/Win32AtomicCompareExchange.h>

// Win32
//
#include <processthreadsapi.h>

// STL
//
#include <iostream>


struct STLWithWin32TaskImpl :
    Cortado::Common::STLAtomicIncDec,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    Cortado::Common::Win32CoroutineScheduler,
    Cortado::Common::Win32AtomicCompareExchange
{
    inline static void YieldCurrentThread()
    {
        YieldProcessor();
    }
};

template <typename R = void>
using Task = Cortado::Task<STLWithWin32TaskImpl, R>;

Task<int> Ans()
{
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";
    co_await Cortado::ResumeBackground();
    std::cout << __FUNCTION__ << " Resumed on thread " << GetCurrentThreadId() << "\n";
    co_return 42;
}

Task<> Ans2()
{
    std::cout << __FUNCTION__ << " Started on thread " << GetCurrentThreadId() << "\n";
    Task<int> tasks[]{ Ans(), Ans(), Ans() };
    co_await Cortado::WhenAll(tasks[0], tasks[1], tasks[2]);
    std::cout << __FUNCTION__ << " Resumed on thread " << GetCurrentThreadId() << "\n";
    co_return;
}

int main()
{
    // Enable memory leak checking at program exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    Ans2().Get();
    return 0;
}