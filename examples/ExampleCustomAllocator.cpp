/// @file ExampleCustomAllocator.h
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

class ExampleCustomAllocator
{
public:
    void* allocate(std::size_t s)
    {
        if (m_currentFrame >= m_frames->size())
        {
            return nullptr;
        }

        m_frameBusy[m_currentFrame] = true;
        return m_frames[m_currentFrame++].data();
    }

    void deallocate(void* ptr, std::size_t)
    {
        for (std::size_t i = 0; i < m_frames->size(); ++i)
        {
            if (ptr == m_frames[i].data())
            {
                m_currentFrame = i;
                m_frameBusy[i] = false;
                return;
            }
        }
    }

    // Consider frame is 1024 bytes of memory
    //
    using static_frame_t = std::array<unsigned char, 1024>;

    // Consider array of frames is 8 sequential frames
    using arr_static_frame_t = std::array<static_frame_t, 8>;

    inline static std::array<std::array<unsigned char, 1024>, 8>* m_frames;
    inline static std::array<bool, 8> m_frameBusy{false};

    inline static std::size_t m_currentFrame = 0;
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

Task<> NothingAsync()
{
    co_await Cortado::ResumeBackground();
}

int main()
{
    ExampleCustomAllocator::m_frames = new std::array<std::array<unsigned char, 1024>, 8>{};
    NothingAsync().Get();
    std::cout << "Expected 0 to be m_currentFrame, actual: " << ExampleCustomAllocator::m_currentFrame << "\n";
    delete(ExampleCustomAllocator::m_frames);
    return 0;
}