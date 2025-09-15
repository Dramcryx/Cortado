/// @file STLExceptionHandler.h
/// Exception handler from STL.
///

#ifndef CORTADO_COMMON_STL_EXCEPTION_HANDLER_H
#define CORTADO_COMMON_STL_EXCEPTION_HANDLER_H

// STL
//
#include <exception>

namespace Cortado::Common
{

/// @brief Concept contract: Defines exception type and functions needed for
/// exception handling.
///
struct STLExceptionHandler
{
    /// @brief Concept contract: stored/rethrown exception type.
    ///
    using Exception = std::exception_ptr;

    /// @brief Concept contract: Exception catcher.
    ///
    inline static std::exception_ptr Catch()
    {
        return std::current_exception();
    }

    /// @brief Concept contract: Exception rethrower.
    /// @param eptr Stored exception.
    ///
    inline static void Rethrow(std::exception_ptr &&eptr)
    {
        std::rethrow_exception(eptr);
    }
};

} // namespace Cortado::Common

#endif
