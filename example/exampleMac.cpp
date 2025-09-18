// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Task.h>

// STL
//
#include <iostream>

using namespace Cortado;

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

Task<> Ans3()
{
    std::cout << __FUNCTION__ << " Started on thread " << pthread_self() << "\n";
    co_await Ans2();
    Task<int> tasks[]{ Ans(), Ans(), Ans() };
    co_await Cortado::WhenAny(tasks[0], tasks[1], tasks[2]);
    std::cout << __FUNCTION__ << " Resumed on thread " << pthread_self() << "\n";
    co_return;
}

int main()
{
    Ans3().Get();
    return 0;
}