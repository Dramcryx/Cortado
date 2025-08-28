/// @file TaskImpl.h
/// Definition of the TaskImpl concept.

#ifndef CORTADO_CONCEPTS_TASK_IMPL_H
#define CORTADO_CONCEPTS_TASK_IMPL_H

// Cortado
//
#include <Cortado/Concepts/Atomic.h>
#include <Cortado/Concepts/CoroutineAllocator.h>
#include <Cortado/Concepts/ErrorHandler.h>
#include <Cortado/Concepts/Event.h>

namespace Cortado::Concepts
{

/// @brief TaskImpl is the core concept of Cortado.
/// A task implementation is something that:<br>
/// 1) Defines how to handle exceptions;<br>
/// 2) Defines an atomic integer (std::atomic_int or substitute);<br>
/// 3) Defines a free atomic cmpxchg operation;<br>
/// 4) Defines yielding function for synchronous wait.
/// @tparam T Candidate for TaskImpl.
///
template <typename T>
concept TaskImpl =
    ErrorHandler<T> && HasAtomic<T> && HasEvent<T> && HasCoroutineAllocator<T>;

} // namespace Cortado::Concepts

#endif
