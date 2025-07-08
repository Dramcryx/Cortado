#ifndef CORTADO_DETAIL_COROUTINE_STORAGE_H
#define CORTADO_DETAIL_COROUTINE_STORAGE_H

// Cortado
//
#include <Cortado/Concepts/AtomicCompareExchange.h>

// STL
//
#include <coroutine>

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

enum class HeldValue : long
{
	None = 0,
	Value,
	Error
};

enum class CallbackRaceState : long
{
	None = 0,
	Value,
	Callback
};

template <typename R, typename E, Concepts::AtomicCompareExchangeFn CmpXchg>
struct CoroutineStorage
{
public:
	CoroutineStorage() = default;

	CoroutineStorage(const CoroutineStorage&) = delete;
	CoroutineStorage& operator=(const CoroutineStorage&) = delete;

	CoroutineStorage(CoroutineStorage&&) = delete;
	CoroutineStorage& operator=(CoroutineStorage&&) = delete;

	~CoroutineStorage()
	{
		DestroyResult();
	}

	template <typename U>
	void SetValue(U&& u)
	{
		new (&m_resultStorage[0]) R{ std::forward<U>(u) };

		m_heldValue = HeldValue::Value;

		CallbackValueRendezvous();
	}

	void SetError(E&& e)
	{
		new (&m_resultStorage[0]) E{ std::forward<E>(e) };

		m_heldValue = HeldValue::Error;

		CallbackValueRendezvous();
	}

	void SetContinuation(std::coroutine_handle<> h)
	{
		m_continuation = h;
		CallbackRaceState expectedState = CallbackRaceState::None;
		if (CmpXchg(
			*reinterpret_cast<volatile long*>(&m_callbackRace),
			*reinterpret_cast<long*>(&expectedState),
			static_cast<long>(CallbackRaceState::Callback)))
		{
			// Successfully stored callback first
			return;
		}
		else if (expectedState == CallbackRaceState::Value)
		{
			m_continuation();
			m_continuation = nullptr;
		}
	}

	HeldValue GetHeldValueType() const
	{
		return m_heldValue;
	}

	R& UnsafeValue()
	{
		return *reinterpret_cast<R*>(&m_resultStorage[0]);
	}

	E& UnsafeError()
	{
		return *reinterpret_cast<E*>(&m_resultStorage[0]);
	}

	std::coroutine_handle<>& Continuation()
	{
		return m_continuation;
	}

private:
	alignas(long) volatile HeldValue m_heldValue = HeldValue::None;
	alignas(long) volatile CallbackRaceState m_callbackRace = CallbackRaceState::None;

	alignas(AlignOfResultStorage<R, E>()) std::byte m_resultStorage[SizeOfResultStorage<R, E>()] = {};

	std::coroutine_handle<> m_continuation = nullptr;

	void DestroyResult()
	{
		switch (m_heldValue)
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

	void CallbackValueRendezvous()
	{
		CallbackRaceState expectedState = CallbackRaceState::None;
		if (CmpXchg(
			*reinterpret_cast<volatile long*>(&m_callbackRace),
			*reinterpret_cast<long*>(&expectedState),
			static_cast<long>(CallbackRaceState::Value)))
		{
			// Successfully stored value first
			return;
		}
		else if (expectedState == CallbackRaceState::Callback)
		{
			// Callback was already set, do the rendezvous
			m_continuation();
			m_continuation = nullptr;
		}
	}
};

} // namespace Cortado::Detail

#endif
