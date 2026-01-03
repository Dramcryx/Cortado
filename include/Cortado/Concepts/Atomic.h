/// @file Atomic.h
/// Concept of atomic variable.
///

#ifndef CORTADO_CONCEPTS_ATOMIC_H
#define CORTADO_CONCEPTS_ATOMIC_H

// STL
//
#include <atomic>
#include <concepts>

namespace Cortado::Concepts
{

/// @brief Atomic primitve used across @link Cortado::Concepts::TaskImpl
/// TaskImpl@endlink.
///
using AtomicPrimitive = std::int64_t;

/// @brief Concept of atomic variable
/// Atomic requires: <br>
/// 0) Being able to store pointer!
/// 1) Construction from any integer which initializes respective value. <br>
/// 2) Atomic pre-increment and pre-decrement. <br>
/// 3) Atomic compare exchange.
/// @tparam T Candidate type for atomicity.
///
template <typename T>
concept Atomic = requires(T t,
                          AtomicPrimitive &expected,
                          AtomicPrimitive desired,
                          std::memory_order memoryOrder) {
    sizeof(AtomicPrimitive) >= sizeof(void *);
    { T{AtomicPrimitive{}} };
    { t.load(memoryOrder) } -> std::same_as<AtomicPrimitive>;
    { t.store(AtomicPrimitive{}, memoryOrder) } -> std::same_as<void>;
    { t.operator++() } -> std::same_as<AtomicPrimitive>;
    { t.operator--() } -> std::same_as<AtomicPrimitive>;
    { t.exchange(desired, memoryOrder) } -> std::same_as<AtomicPrimitive>;
    {
        t.compare_exchange_strong(expected, desired, memoryOrder, memoryOrder)
    } -> std::same_as<bool>;
    {
        t.compare_exchange_weak(expected, desired, memoryOrder, memoryOrder)
    } -> std::same_as<bool>;
};

/// @brief Extension for Atomic which requires wait/notify methods
///
template <typename T>
concept FutexLikeAtomic =
    Atomic<T> && requires(T t, AtomicPrimitive old, std::memory_order mo) {
        { t.wait(old, mo) } -> std::same_as<void>;
        { t.notify_one() } -> std::same_as<void>;
        { t.notify_all() } -> std::same_as<void>;
    };

/// @brief Helper concept to define if T defines
/// Atomic type (via `using` or `typedef`), and that type
/// satisifes @link Cortado::Concepts::Atomic Atomic@endlink concept constaints.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept HasAtomic = requires {
    // std::atomic_int64_t or substitute
    //
    typename T::Atomic;
} && Atomic<typename T::Atomic>;

/// @brief Helper concept to define if T defines
/// Atomic type (via `using` or `typedef`), and that type
/// satisifes @link Cortado::Concepts::Atomic Atomic@endlink and @link
/// Cortado::Concepts::FutexLikeAtomic FutexLikeAtomic@endlink concepts
/// constaints.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept HasFutexLikeAtomic =
    HasAtomic<T> && FutexLikeAtomic<typename T::Atomic>;

} // namespace Cortado::Concepts

#endif
