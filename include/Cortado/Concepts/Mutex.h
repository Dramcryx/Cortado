/// @file Mutex.h
/// Definition of a mutex concept.
///

#ifndef CORTADO_CONCEPTS_MUTEX_H
#define CORTADO_CONCEPTS_MUTEX_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

/// @brief Mutex concept is described to match STL mutex.
///
template <typename MutexImplT>
concept Mutex = requires(MutexImplT mx) {
    { mx.lock() } -> std::same_as<void>;
    { mx.try_lock() } -> std::same_as<bool>;
    { mx.unlock() } -> std::same_as<void>;
};

} // namespace Cortado::Concepts

#endif // CORTADO_CONCEPTS_MUTEX_H
