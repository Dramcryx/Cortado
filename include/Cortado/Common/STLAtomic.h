#ifndef CORTADO_COMMON_STL_ATOMIC_INC_DEC_H
#define CORTADO_COMMON_STL_ATOMIC_INC_DEC_H

// STL
//
#include <atomic>

namespace Cortado::Common
{

struct STLAtomic
{
    using Atomic = std::atomic<unsigned long>;
};

} // namespace Cortado::Common

#endif
