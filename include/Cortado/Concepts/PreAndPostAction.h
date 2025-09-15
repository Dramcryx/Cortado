/// @file PreAndPostAction.h
/// Definition of the PreAndPostAction concept.
///

#ifndef CORTADO_CONCEPTS_PRE_AND_POST_ACTION_H
#define CORTADO_CONCEPTS_PRE_AND_POST_ACTION_H

// STL
//
#include <concepts>
#include <type_traits>

namespace Cortado::Concepts
{

/// @brief Helper concept to define if T defines
/// a type for additional storage. It is currently required to be
/// default-constructible.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept HasAdditionalStorage = requires {
    typename T::AdditionalStorage;
    std::is_default_constructible_v<typename T::AdditionalStorage>;
};

/// @brief Concept that allows user to extend or modify behavior of suspend and
/// resume.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
template <typename T>
concept PreAndPostAction =
    HasAdditionalStorage<T> &&
    requires(typename T::AdditionalStorage &additionalStorage) {
        { T::OnBeforeSuspend(additionalStorage) } -> std::same_as<void>;
        { T::OnBeforeResume(additionalStorage) } -> std::same_as<void>;
    };

} // namespace Cortado::Concepts

#endif
