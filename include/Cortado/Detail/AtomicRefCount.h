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
    Concepts::AtomicPrimitive AddRef() noexcept
    {
        return ++m_refCount;
    }

    Concepts::AtomicPrimitive Release() noexcept
    {
        return --m_refCount;
    }

private:
    T m_refCount{1};
};

} // namespace Cortado::Detail

#endif
