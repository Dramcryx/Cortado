#ifndef CORTADO_CONCEPTS_EVENT_H
#define CORTADO_CONCEPTS_EVENT_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

template <typename T>
concept Event = requires(T t)
{
    { t.Wait() } -> std::same_as<void>;
    { t.Singal() } -> std::same_as<void>;
};

} // namespace Cortado::Concepts

#endif
