/// @file CoroutineAllocator.h
/// Definition of the CoroutineAllocator concept.
///

#ifndef CORTADO_CONCEPTS_ALLOCATOR_H
#define CORTADO_CONCEPTS_ALLOCATOR_H

// STL
//
#include <concepts>
#include <cstddef>

namespace Cortado::Concepts
{

/// @brief Coroutine allocator concept serves only allocation.
/// A general recommendation to custom allocator implementation is
/// to be ref-counting proxy. This allows for quick copy operations
/// which are required by compiler when picking a `new` overload.
///
template <typename T>
concept CoroutineAllocator = requires(T t, void *p, std::size_t s) {
    std::is_copy_constructible_v<T>;
    { t.allocate(s) } -> std::same_as<void *>;
    { t.deallocate(p, s) } -> std::same_as<void>;
};

/// @brief Helper concept to define if T defines
/// @link Cortado::Concepts::CoroutineAllocator CoroutineAllocator@endlink type.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept HasCoroutineAllocator = requires {
    // Allocator type is defined
    //
    typename T::Allocator;
} && CoroutineAllocator<typename T::Allocator>;

} // namespace Cortado::Concepts

#endif
