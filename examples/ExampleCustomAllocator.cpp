/// @file ExampleCustomAllocator.cpp
/// Example of customizing an allocator.
///

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/DefaultTaskImpl.h>

// STL
//
#include <array>
#include <cstdlib>
#include <iostream>

// Fake the memory by pre-allocating 8 times 1024 bytes
//
struct FakeFrames
{
    // Consider frame is 1024 bytes of memory
    //
    using static_frame_t = std::array<unsigned char, 1024>;

    // Consider array of frames is 8 sequential frames
    //
    using arr_static_frame_t = std::array<static_frame_t, 8>;

    // Our fake memory that we will pretend to allocate
    //
    alignas(8) std::array<std::array<unsigned char, 1024>, 8> Frames;
    std::array<bool, 8> FrameBusiness{false};

    std::size_t CurrentFrame = 0;

    void* AllocateNext()
    {
        if (CurrentFrame >= Frames.size())
        {
            return nullptr;
        }

        FrameBusiness[CurrentFrame] = true;
        return Frames[CurrentFrame++].data();
    }

    void Deallocate(void* ptr)
    {
        for (std::size_t i = 0; i < Frames.size(); ++i)
        {
            if (ptr == Frames[i].data())
            {
                CurrentFrame = i;
                FrameBusiness[i] = false;
                return;
            }
        }
    }
};

// Custom allocator that allocates from pre-allocated fake memory
// It will hold a shared_ptr to FakeFrames so that we can print some statistics
// later when this instance is moved into coroutine storage.
//
class ExampleCustomAllocator
{
public:
    std::shared_ptr<FakeFrames> FakeFramesPtr;

    void *allocate(std::size_t s)
    {
        return FakeFramesPtr->AllocateNext();
    }

    void deallocate(void *ptr, std::size_t)
    {
        FakeFramesPtr->Deallocate(ptr);
    }
};

struct TaskImplWithCustomAllocator :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLExceptionHandler,
    Cortado::DefaultScheduler
{
    using Event = Cortado::DefaultEvent;
    using Allocator = ExampleCustomAllocator;
};

template <typename T = void>
using Task = Cortado::Task<T, TaskImplWithCustomAllocator>;

Task<> NothingAsync(ExampleCustomAllocator)
{
    co_await Cortado::ResumeBackground();
}

int main()
{
    auto frames = std::make_shared<FakeFrames>();

    ExampleCustomAllocator allocator{frames};
    auto task = NothingAsync(allocator);
    std::cout << "Expected 1 to be CurrentFrame, actual: "
              << frames->CurrentFrame << "\n";

    auto task2 = NothingAsync(allocator);
    std::cout << "Expected 2 to be CurrentFrame, actual: "
              << frames->CurrentFrame << "\n";

    task2.Wait();
    // scope-out second task
    {
        Task<>{std::move(task2)};
    }
    std::cout << "Expected 1 to be CurrentFrame, actual: "
              << frames->CurrentFrame << "\n";

    task.Wait();
    // scope-out first task
    {
        Task<>{std::move(task)};
    }
    std::cout << "Expected 0 to be CurrentFrame, actual: "
              << frames->CurrentFrame << "\n";
    return 0;
}
