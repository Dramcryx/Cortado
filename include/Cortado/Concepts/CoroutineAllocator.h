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

} // namespace Cortado::Concepts

#endif
