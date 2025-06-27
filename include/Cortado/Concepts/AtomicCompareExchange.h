#ifndef CORTADO_CONCEPTS_ATOMIC_COMPARE_EXCHANGE_H
#define CORTADO_CONCEPTS_ATOMIC_COMPARE_EXCHANGE_H

namespace Cortado::Concepts
{

using AtomicCompareExchangeFn = bool(*)(volatile long& /*v*/, long& /*expected*/, long /*desired*/);

template <typename T>
concept HasAtomicCompareExchangeFn = requires
{
	// Atomic cmpxchg function
	//
	{ T::AtomicCompareExchangeFn } -> std::convertible_to<AtomicCompareExchangeFn>;
};

} // namespace Cortado::Concepts

#endif
