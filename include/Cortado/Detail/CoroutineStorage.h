#ifndef CORTADO_DETAIL_COROUTINE_STORAGE_H
#define CORTADO_DETAIL_COROUTINE_STORAGE_H

// Cortado
//
#include <Cortado/Concepts/Atomic.h>

namespace Cortado::Detail
{

template <typename R, typename E>
constexpr std::size_t AlignOfResultStorage()
{
    constexpr auto AlignOfR = alignof(R);
    constexpr auto AlignOfE = alignof(E);

    return AlignOfR > AlignOfE ? AlignOfR : AlignOfE;
}

template <typename R, typename E>
constexpr std::size_t SizeOfResultStorage()
{
    constexpr auto SizeOfR = sizeof(R);
    constexpr auto SizeOfE = sizeof(E);

    return SizeOfR > SizeOfE ? SizeOfR : SizeOfE;
}

enum class HeldValue : Concepts::AtomicPrimitive
{
    None = 0,
    Value,
    Error
};

template <typename R, typename E, Concepts::Atomic Atomic>
struct CoroutineStorage
{
public:
    CoroutineStorage() = default;

    CoroutineStorage(const CoroutineStorage &) = delete;
    CoroutineStorage &operator=(const CoroutineStorage &) = delete;

    CoroutineStorage(CoroutineStorage &&) = delete;
    CoroutineStorage &operator=(CoroutineStorage &&) = delete;

    ~CoroutineStorage()
    {
        DestroyResult();
    }

    template <typename U>
    void SetValue(U &&u)
    {
        ::new (&m_resultStorage[0]) R{std::forward<U>(u)};

        m_heldValue = static_cast<Concepts::AtomicPrimitive>(HeldValue::Value);
    }

    void SetError(E &&e)
    {
        ::new (&m_resultStorage[0]) E{std::forward<E>(e)};

        m_heldValue = static_cast<Concepts::AtomicPrimitive>(HeldValue::Error);
    }

    HeldValue GetHeldValueType() const
    {
        Concepts::AtomicPrimitive valueType = m_heldValue;
        return static_cast<HeldValue>(valueType);
    }

    R &UnsafeValue()
    {
        return *reinterpret_cast<R *>(&m_resultStorage[0]);
    }

    E &UnsafeError()
    {
        return *reinterpret_cast<E *>(&m_resultStorage[0]);
    }

private:
    Atomic m_heldValue{static_cast<Concepts::AtomicPrimitive>(HeldValue::None)};

    alignas(AlignOfResultStorage<R, E>()) std::byte
        m_resultStorage[SizeOfResultStorage<R, E>()] = {};

    void DestroyResult()
    {
        Concepts::AtomicPrimitive valueType = m_heldValue;
        switch (static_cast<HeldValue>(valueType))
        {
        case HeldValue::Value:
            UnsafeValue().~R();
            break;
        case HeldValue::Error:
            UnsafeError().~E();
            break;
        default:
            break;
        }
    }
};

} // namespace Cortado::Detail

#endif
