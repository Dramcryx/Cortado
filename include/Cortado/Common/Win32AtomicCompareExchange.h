#ifndef CORTADO_COMMON_INTERLOCKED_COMPARE_EXCHANGE_H
#define CORTADO_COMMON_INTERLOCKED_COMPARE_EXCHANGE_H

// Win32
//
#include <intrin.h>

namespace Cortado::Common
{

struct Win32AtomicCompareExchange
{
    static bool Win32AtomicCompareExchangeFn(volatile long& obj, long& expected, long desired)
    {
        long original = _InterlockedCompareExchange(
            reinterpret_cast<volatile long*>(&obj),
            desired,
            expected
        );

        if (original == expected)
        {
            return true;
        }
        else
        {
            expected = original; // Update caller's expected value
            return false;
        }
    }

    static constexpr auto AtomicCompareExchangeFn = Win32AtomicCompareExchangeFn;
};

} // namespace Cortado::Common

#endif
