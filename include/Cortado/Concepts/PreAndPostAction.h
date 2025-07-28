#ifndef CORTADO_CONCEPTS_PRE_AND_POST_ACTION_H
#define CORTADO_CONCEPTS_PRE_AND_POST_ACTION_H

// STL
//
#include <concepts>
#include <type_traits>

namespace Cortado::Concepts
{

template <typename T>
concept HasAdditionalStorage = requires {
    typename T::AdditionalStorage;
    std::is_default_constructible_v<typename T::AdditionalStorage>;
};

template <typename T>
concept PreAndPostAction =
    HasAdditionalStorage<T> &&
    requires(typename T::AdditionalStorage &additionalStorage) {
        { T::OnBeforeSuspend(additionalStorage) } -> std::same_as<void>;
        { T::OnBeforeResume(additionalStorage) } -> std::same_as<void>;
    };

} // namespace Cortado::Concepts

#endif
