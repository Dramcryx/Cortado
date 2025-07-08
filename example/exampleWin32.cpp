// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Task.h>

// STL
//
#include <iostream>

using namespace Cortado;

class WithAsyncMethod
{
public:
    Task<int> VoidAsync()
    {
        co_return 1;
    }
};

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
    Task<int> tasks[]{ Ans(), Ans(), Ans(), WithAsyncMethod{}.VoidAsync() };
    co_await Cortado::WhenAll(tasks[0], tasks[1], tasks[2], tasks[3]);
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