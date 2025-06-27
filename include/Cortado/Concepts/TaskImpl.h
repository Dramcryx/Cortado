#ifndef CORTADO_CONCEPTS_TASK_IMPL_H
#define CORTADO_CONCEPTS_TASK_IMPL_H

// Cortado
//
#include <Cortado/Concepts/AtomicIncDec.h>
#include <Cortado/Concepts/AtomicCompareExchange.h>
#include <Cortado/Concepts/ErrorHandler.h>

namespace Cortado::Concepts
{

// A task implementation is something that:
// 1) Defines how to handle exceptions;
// 2) Defines an atomic integer (std::atomic_int or substitute);
// 3) Defines a free atomic cmpxchg operation;
// 4) Defines yielding function for synchronous wait.
//
template <typename T>
concept TaskImpl =
	ErrorHandler<T>
	&& HasAtomicIncDec<T>
	&& HasAtomicCompareExchangeFn<T>
	&& requires
	{
		// Thread yielder func
		//
		{ T::YieldCurrentThread() };
	};

} // namespace Cortado::Concepts

#endif
