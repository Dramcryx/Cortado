/// @file ErrorHandler.h
/// Definition of the ErrorHandler concept.
///

#ifndef CORTADO_CONCEPTS_ERROR_HANDLER_H
#define CORTADO_CONCEPTS_ERROR_HANDLER_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

/// @brief ErrorHandler should define the exception type,
/// core logic for unhandled_exception, logic catch an exception
/// and how to rethrow it.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink type.
///
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
