/// @file STLAtomic.h
/// Atomic primitive from STL.
///

#ifndef CORTADO_COMMON_STL_ATOMIC_INC_DEC_H
#define CORTADO_COMMON_STL_ATOMIC_INC_DEC_H

// STL
//
#include <atomic>

namespace Cortado::Common
{

/// @brief Struct which defines atomic primitive implementation.
///
struct STLAtomic
{
    using Atomic = std::atomic_ulong;
};

} // namespace Cortado::Common

#endif
