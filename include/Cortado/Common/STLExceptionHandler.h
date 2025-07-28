#ifndef CORTADO_COMMON_STL_EXCEPTION_HANDLER_H
#define CORTADO_COMMON_STL_EXCEPTION_HANDLER_H

// STL
//
#include <exception>

namespace Cortado::Common
{

struct STLExceptionHandler
{
    using Exception = std::exception_ptr;

    inline static std::exception_ptr Catch()
    {
        return std::current_exception();
    }

    inline static void Rethrow(std::exception_ptr &&eptr)
    {
        std::rethrow_exception(eptr);
    }
};

} // namespace Cortado::Common

#endif
