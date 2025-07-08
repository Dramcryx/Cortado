#ifndef CORTADO_ATOMIC_COMPARE_EXCHANGE_WIN32_ATOMIC_COMPARE_EXCHANGE_H
#define CORTADO_ATOMIC_COMPARE_EXCHANGE_WIN32_ATOMIC_COMPARE_EXCHANGE_H

// Win32
//
#include <intrin.h>

namespace Cortado::AtomicCompareExchange
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

} // namespace Cortado::AtomicCompareExchange

#endif
