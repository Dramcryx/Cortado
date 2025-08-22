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

/// @brief Atomic primitve used across TaskImpl.
///
using AtomicPrimitive = unsigned long;

/// @brief Concept of atomic variable
/// Atomic requires:
/// 1) Construction from any integer which initializes respective value;
/// 2) Atomic pre-increment and pre-decrement.
/// 3) Atomic compare exchange.
/// @tparam T Candidate type for atomicity.
///
template <typename T>
concept Atomic =
    requires(T t, AtomicPrimitive &expected, AtomicPrimitive desired) {
        { T{1} };
        { t.operator++() } -> std::same_as<AtomicPrimitive>;
        { t.operator--() } -> std::same_as<AtomicPrimitive>;
        { t.compare_exchange_strong(expected, desired) } -> std::same_as<bool>;
    };

/// @brief Helper concept to define if `T` defines
/// `Atomic` type (via `using` or `typedef`), and that type
/// suits `Atomic` concept constaints.
/// @tparam T TaskImpl type.
///
template <typename T>
concept HasAtomic = requires {
    // std::atomic_int or substitute
    //
    typename T::Atomic;

    // T::Atomic satisfies concept
    //
    Atomic<typename T::Atomic>;
};

} // namespace Cortado::Concepts

#endif
