/// @file CoroutineStorage.h
/// Value/exception storage.
///

#ifndef CORTADO_DETAIL_COROUTINE_STORAGE_H
#define CORTADO_DETAIL_COROUTINE_STORAGE_H

// Cortado
//
#include <Cortado/Concepts/Atomic.h>

namespace Cortado::Detail
{

/// @brief Helepr `constexpr` function to define largest requiret alignment.
/// @returns Alignment in bytes for storage class.
///
template <typename R, typename E>
constexpr std::size_t AlignOfResultStorage()
{
    constexpr auto AlignOfR = alignof(R);
    constexpr auto AlignOfE = alignof(E);

    return AlignOfR > AlignOfE ? AlignOfR : AlignOfE;
}

/// @brief Helepr `constexpr` function to define largest requiret size.
/// @returns Size in bytes for storage class.
///
template <typename R, typename E>
constexpr std::size_t SizeOfResultStorage()
{
    constexpr auto SizeOfR = sizeof(R);
    constexpr auto SizeOfE = sizeof(E);

    return SizeOfR > SizeOfE ? SizeOfR : SizeOfE;
}

/// @brief Describes current storage state.
///
enum class HeldValue : Concepts::AtomicPrimitive
{
    None = 0,
    Value,
    Error
};

/// @brief Coroutine value/exception storage class. It is as primitive as
/// possible, only manages get-set operations on value/error and provides
/// querying current state.
/// @tparam R Value type.
/// @tparam E Exception type.
/// @tparam Atomic Implementation of atomic primitive.
///
template <typename R, typename E, Concepts::Atomic Atomic>
struct CoroutineStorage
{
public:
    /// @brief Default constructor.
    ///
    CoroutineStorage() = default;

    /// @brief Non-copyable.
    ///
    CoroutineStorage(const CoroutineStorage &) = delete;

    /// @brief Non-copyable.
    ///
    CoroutineStorage &operator=(const CoroutineStorage &) = delete;

    /// @brief Non-movable.
    ///
    CoroutineStorage(CoroutineStorage &&) = delete;

    /// @brief Non-movable.
    ///
    CoroutineStorage &operator=(CoroutineStorage &&) = delete;

    /// @brief Destructor. Destructs current value/exception if any.
    ///
    ~CoroutineStorage()
    {
        DestroyResult();
    }

    /// @brief Constructs value in storage.
    /// @tparam U Same as R or convertible to it.
    ///
    template <typename U>
    void SetValue(U &&u)
    {
        ::new (&m_resultStorage[0]) R{std::forward<U>(u)};

        m_heldValue = static_cast<Concepts::AtomicPrimitive>(HeldValue::Value);
    }

    /// @brief Constructs exception in storage.
    /// @param e Exception to store.
    ///
    void SetError(E &&e)
    {
        ::new (&m_resultStorage[0]) E{std::forward<E>(e)};

        m_heldValue = static_cast<Concepts::AtomicPrimitive>(HeldValue::Error);
    }

    /// @brief Get current storage state.
    /// @returns Storage state value.
    ///
    HeldValue GetHeldValueType() const
    {
        Concepts::AtomicPrimitive valueType = m_heldValue;
        return static_cast<HeldValue>(valueType);
    }

    /// @brief Get reference to value without checking current state.
    /// @returns Reference to `R`.
    ///
    R &UnsafeValue()
    {
        return *reinterpret_cast<R *>(&m_resultStorage[0]);
    }

    /// @brief Get reference to exception without checking current state.
    /// @returns Reference to `E`.
    ///
    E &UnsafeError()
    {
        return *reinterpret_cast<E *>(&m_resultStorage[0]);
    }

private:
    /// @brief Current state value.
    ///
    Atomic m_heldValue{static_cast<Concepts::AtomicPrimitive>(HeldValue::None)};

    /// @brief Value/exception storage.
    ///
    alignas(AlignOfResultStorage<R, E>()) std::byte
        m_resultStorage[SizeOfResultStorage<R, E>()] = {};

    /// @brief Call destructor for stored object if any.
    ///
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
