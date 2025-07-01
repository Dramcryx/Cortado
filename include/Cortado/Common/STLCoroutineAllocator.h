#ifndef CORTADO_COMMON_STL_COROUTINE_ALLOCATOR_H
#define CORTADO_COMMON_STL_COROUTINE_ALLOCATOR_H

// STL
//
#include <memory>

namespace Cortado::Common
{

struct STLAllocator
{
    std::allocator<std::byte> m_allocator;

    void* allocate(std::size_t size)
    {
        return m_allocator.allocate(size);
    }
};

struct STLCoroutineAllocator
{
    using Allocator = STLAllocator;
};

} // namespace Cortado::Common

#endif