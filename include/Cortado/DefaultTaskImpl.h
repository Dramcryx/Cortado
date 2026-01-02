/// @brief DefaultTaskImpl.h
/// A default task implementation for Win32 and Mac.
///

#ifndef CORTADO_DEFAULT_TASK_IMPL_H
#define CORTADO_DEFAULT_TASK_IMPL_H

// Cortado
//
#include <Cortado/Common/STLAtomic.h>
#include <Cortado/Common/STLCoroutineAllocator.h>
#include <Cortado/Common/STLExceptionHandler.h>
#include <Cortado/DefaultEvent.h>
#include <Cortado/DefaultScheduler.h>

namespace Cortado
{

/// @brief Default implementation with platform-default scheduler,
/// STL allocator, atomic and exception.
///
struct DefaultTaskImpl :
    Common::STLAtomic,
    Common::STLCoroutineAllocator,
    Common::STLExceptionHandler,
    DefaultScheduler
{
    using Event = DefaultEvent;
};

} // namespace Cortado

#endif
