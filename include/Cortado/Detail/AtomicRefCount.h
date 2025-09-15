/// @file AtomicRefCount.h
/// Simple atomic refcounter.
///

#ifndef CORTADO_DETAIL_ATOMIC_REF_COUNT_H
#define CORTADO_DETAIL_ATOMIC_REF_COUNT_H

// Cortado
//
#include <Cortado/Concepts/Atomic.h>

namespace Cortado::Detail
{

/// @brief Implements basic refcount needed for promise_type lifetime tracking.
/// @tparam T Implementation of atomic primitive.
///
template <Concepts::Atomic T>
class AtomicRefCount
{
public:
    /// @brief Increment reference count.
    ///
    Concepts::AtomicPrimitive AddRef() noexcept
    {
        return ++m_refCount;
    }

    /// @brief Decrement reference count.
    ///
    Concepts::AtomicPrimitive Release() noexcept
    {
        return --m_refCount;
    }

private:
    T m_refCount{1};
};

} // namespace Cortado::Detail

#endif
