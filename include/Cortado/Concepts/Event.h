/// @file Event.h
/// Definition of the Event concept.
///

#ifndef CORTADO_CONCEPTS_EVENT_H
#define CORTADO_CONCEPTS_EVENT_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

/// @brief Event should implement logic for busy wait of some operation.
/// It is used for sync wait of the task if the value is explicitly requested,
/// and coroutine is not finished yet.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept Event = requires(T t, unsigned long timeToWaitMs) {
    { t.Wait() } -> std::same_as<void>;
    { t.WaitFor(timeToWaitMs) } -> std::same_as<bool>;
    { t.Set() } -> std::same_as<void>;
    { t.IsSet() } -> std::same_as<bool>;
};

/// @brief Helper concept to define if T defines
/// @link Cortado::Concepts::Event Event@endlink type.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept HasEvent = requires {
    // Event type is defined
    //
    typename T::Event;

    // T::Event satisfies concept
    //
    Event<typename T::Event>;
};

} // namespace Cortado::Concepts

#endif
