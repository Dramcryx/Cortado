#ifndef CORTADO_COMMON_STL_ATOMIC_INC_DEC_H
#define CORTADO_COMMON_STL_ATOMIC_INC_DEC_H

// STL
//
#include <atomic>

namespace Cortado::Common
{

struct STLAtomicIncDec
{
	using AtomicIncDec = std::atomic<int>;
};

} // namespace Cortado::Common

#endif