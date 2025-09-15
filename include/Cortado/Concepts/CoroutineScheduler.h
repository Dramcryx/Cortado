/// @file CoroutineScheduler.h
/// Definition of the CoroutineScheduler concept.
///

#ifndef CORTADO_CONCEPTS_COROUTINE_SCHEDULER_H
#define CORTADO_CONCEPTS_COROUTINE_SCHEDULER_H

// STL
//
#include <coroutine>

namespace Cortado::Concepts
{

/// @brief CoroutineScheduler is something that allows for scheduling
/// coroutines.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept CoroutineScheduler =
    requires(std::remove_reference_t<T> t, std::coroutine_handle<> h) {
        { t.Schedule(h) };
    };

} // namespace Cortado::Concepts

#endif