#ifndef CORTADO_CONCEPTS_ALLOCATOR_H
#define CORTADO_CONCEPTS_ALLOCATOR_H

// STL
//
#include <concepts>
#include <cstddef>

namespace Cortado::Concepts
{

template <typename T>
concept CoroutineAllocator = requires(T t, void *p, std::size_t s) {
    { t.allocate(s) } -> std::same_as<void *>;
};

template <typename T>
concept HasCoroutineAllocator = requires {
    // Allocator type is defined
    //
    typename T::Allocator;

    // T::Allocator satisfies concept
    //
    CoroutineAllocator<typename T::Allocator>;
};

} // namespace Cortado::Concepts

#endif
