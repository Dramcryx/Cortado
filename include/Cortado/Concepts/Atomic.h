#ifndef CORTADO_CONCEPTS_ATOMIC_H
#define CORTADO_CONCEPTS_ATOMIC_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

// Atomic requires:
// 1) Construction from any integer which initializes respective value;
// 2) Atomic pre-increment and pre-decrement.
// 3) Atomic compare exchange.
//
template <typename T>
concept Atomic = requires(T t, unsigned long &expected, unsigned long desired) { 
    { T{1} };
    { t.operator++() } -> std::same_as<unsigned long>;
    { t.operator--() } -> std::same_as<unsigned long>;
    { t.compare_exchange_strong(expected, desired) } -> std::same_as<bool>;
};

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
