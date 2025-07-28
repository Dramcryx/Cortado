#ifndef CORTADO_CONCEPTS_ERROR_HANDLER_H
#define CORTADO_CONCEPTS_ERROR_HANDLER_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

template <typename T>
concept ErrorHandler = requires {
    // The type that is caught and thrown
    //
    typename T::Exception;

    // Static catcher which returns exception
    //
    { T::Catch() } -> std::same_as<typename T::Exception>;

    // Statis rethrower which rethrows previously caught value
    //
    { T::Rethrow(T::Catch()) } -> std::same_as<void>;
};

} // namespace Cortado::Concepts

#endif
