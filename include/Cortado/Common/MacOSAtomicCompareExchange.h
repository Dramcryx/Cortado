#ifndef CORTADO_COMMON_MACOS_ATOMIC_COMPARE_EXCHANGE_H
#define CORTADO_COMMON_MACOS_ATOMIC_COMPARE_EXCHANGE_H

// MacOS
//
#include <libkern/OSAtomic.h>

namespace Cortado::Common
{

struct MacOSAtomicCompareExchange
{
    static bool MacOSAtomicCompareExchangeFn(volatile long& obj, long& expected, long desired)
    {
        long old = expected;
        if (OSAtomicCompareAndSwap64((int64_t)old, (int64_t)desired, (volatile int64_t*)&obj)) {
            return true;
        } else {
            expected = obj;
            return false;
        }
    }

    static constexpr auto AtomicCompareExchangeFn = MacOSAtomicCompareExchangeFn;
};

} // namespace Cortado::Common

#endif
