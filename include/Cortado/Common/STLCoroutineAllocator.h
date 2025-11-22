/// @file STLCoroutineAllocator.h
/// Allocator from STL.
///

#ifndef CORTADO_COMMON_STL_COROUTINE_ALLOCATOR_H
#define CORTADO_COMMON_STL_COROUTINE_ALLOCATOR_H

// STL
//
#include <memory>

namespace Cortado::Common
{

/// @brief Allocator implementation which is basically STL allocator of bytes.
///
struct STLAllocator
{
    std::allocator<std::byte> m_allocator;

    /// @brief Concept contract: Allocates requested amount of bytes.
    ///
    void *allocate(std::size_t size)
    {
        return m_allocator.allocate(size);
    }

    /// @brief Concept contract: Deallocates requested pointer.
    ///
    void deallocate(void* ptr, std::size_t size)
    {
        m_allocator.deallocate(reinterpret_cast<std::byte*>(ptr), size);
    }
};

/// @brief Struct that defines allocator type used for task implementation.
///
struct STLCoroutineAllocator
{
    using Allocator = STLAllocator;
};

} // namespace Cortado::Common

#endif
