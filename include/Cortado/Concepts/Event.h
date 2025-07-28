#ifndef CORTADO_CONCEPTS_EVENT_H
#define CORTADO_CONCEPTS_EVENT_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

template <typename T>
concept Event = requires(T t, unsigned long timeToWaitMs) {
    { t.Wait() } -> std::same_as<void>;
    { t.WaitFor(timeToWaitMs) } -> std::same_as<bool>;
    { t.Set() } -> std::same_as<void>;
    { t.IsSet() } -> std::same_as<bool>;
};

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
