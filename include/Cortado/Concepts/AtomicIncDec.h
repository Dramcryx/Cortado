#ifndef CORTADO_CONCEPTS_ATOMIC_INC_DEC_H
#define CORTADO_CONCEPTS_ATOMIC_INC_DEC_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

// AtomicIncDec requires:
// 1) Construction from any integer which initializes respective value;
// 2) Atomic pre-increment and pre-decrement.
//
template <typename T>
concept AtomicIncDec = requires (T t)
{
    { T{ 1 } };
    { t.operator++() } -> std::same_as<int>;
    { t.operator--() } -> std::same_as<int>;
};

template <typename T>
concept HasAtomicIncDec = requires
{
	// std::atomic_int or substitute
	//
	typename T::AtomicIncDec;

	// T::AtomicIncDec satisfies concept
	//
	AtomicIncDec<typename T::AtomicIncDec>;
};

} // namespace Cortado::Concepts

#endif
