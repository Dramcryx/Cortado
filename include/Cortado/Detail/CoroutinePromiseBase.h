#ifndef CORTADO_COROUTINE_PROMISE_BASE_H
#define CORTADO_COROUTINE_PROMISE_BASE_H

// Cortado
//
#include <Cortado/Concepts/TaskImpl.h>
#include <Cortado/Detail/AtomicRefCount.h>
#include <Cortado/Detail/CoroutineStorage.h>

namespace Cortado::Detail
{

template <Concepts::TaskImpl T, typename R>
struct CoroutinePromiseBase : AtomicRefCount<typename T::AtomicIncDec>
{
	std::suspend_never initial_suspend()
	{
		return {};
	}

	decltype(auto) final_suspend() noexcept
	{
		struct FinalAwaiter
		{
			bool await_ready() noexcept
			{
				return _this.Release() == 0;
			}

			std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept
			{
				auto next = _this.m_storage.Continuation();
				if (next == nullptr)
				{
					return std::noop_coroutine();
				}

				return next;
			}

			void await_resume() noexcept
			{
			}

			CoroutinePromiseBase& _this;
		};

		return FinalAwaiter{ *this };
	}

	void unhandled_exception()
	{
		m_storage.SetError(T::Catch());
	}

	bool Ready()
	{
		return m_storage.GetHeldValueType() != HeldValue::None;
	}

	void SetContinuation(std::coroutine_handle<> h)
	{
		m_storage.SetContinuation(h);
	}

protected:
	CoroutineStorage<R, typename T::Exception, T::AtomicCompareExchangeFn> m_storage;

	void RethrowError()
	{
		if (m_storage.GetHeldValueType() == HeldValue::Error)
		{
			T::Rethrow(std::move(m_storage.UnsafeError()));
		}
	}
};

template <Concepts::TaskImpl T, typename R>
struct CoroutinePromiseBaseWithValue : CoroutinePromiseBase<T, R>
{
	template <typename U>
	void return_value(U&& u)
	{
		this->m_storage.SetValue(std::forward<U>(u));
	}

	decltype(auto) Get()
	{
		this->RethrowError();
		return std::move(this->m_storage.UnsafeValue());
	}
};

template <Concepts::TaskImpl T>
struct CoroutinePromiseBaseWithValue<T, void> : CoroutinePromiseBase<T, bool>
{
	void return_void()
	{
		this->m_storage.SetValue(true);
	}

	void Get()
	{
		this->RethrowError();
	}
};

} // namespace Cortado::Detail

#endif
