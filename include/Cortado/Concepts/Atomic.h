/// @file Atomic.h
/// Concept of atomic variable.
///

#ifndef CORTADO_CONCEPTS_ATOMIC_H
#define CORTADO_CONCEPTS_ATOMIC_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

/// @brief Atomic primitve used across @link Cortado::Concepts::TaskImpl
/// TaskImpl@endlink.
///
using AtomicPrimitive = unsigned long;

/// @brief Concept of atomic variable
/// Atomic requires: <br>
/// 1) Construction from any integer which initializes respective value; <br>
/// 2) Atomic pre-increment and pre-decrement. <br>
/// 3) Atomic compare exchange.
/// @tparam T Candidate type for atomicity.
///
template <typename T>
concept Atomic =
    requires(T t, AtomicPrimitive &expected, AtomicPrimitive desired) {
        { T{AtomicPrimitive{}} };
        { t.load() } -> std::same_as<AtomicPrimitive>;
        { t.store(AtomicPrimitive{}) } -> std::same_as<void>;
        { t.operator++() } -> std::same_as<AtomicPrimitive>;
        { t.operator--() } -> std::same_as<AtomicPrimitive>;
        { t.compare_exchange_strong(expected, desired) } -> std::same_as<bool>;
    };

/// @brief Helper concept to define if T defines
/// Atomic type (via `using` or `typedef`), and that type
/// suits @link Cortado::Concepts::Atomic Atomic@endlink concept constaints.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept HasAtomic = requires {
    // std::atomic_ulong or substitute
    //
    typename T::Atomic;
} && Atomic<typename T::Atomic>;

} // namespace Cortado::Concepts

#endif
