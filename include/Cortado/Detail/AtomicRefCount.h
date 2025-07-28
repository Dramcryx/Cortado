#ifndef CORTADO_DETAIL_ATOMIC_REF_COUNT_H
#define CORTADO_DETAIL_ATOMIC_REF_COUNT_H

// Cortado
//
#include <Cortado/Concepts/Atomic.h>

namespace Cortado::Detail
{

template <Concepts::Atomic T>
class AtomicRefCount
{
public:
    unsigned long AddRef() noexcept
    {
        return ++m_refCount;
    }

    unsigned long Release() noexcept
    {
        return --m_refCount;
    }

private:
    T m_refCount{1};
};

} // namespace Cortado::Detail

#endif
