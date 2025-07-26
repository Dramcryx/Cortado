#ifndef CORTADO_COROUTINE_PROMISE_BASE_H
#define CORTADO_COROUTINE_PROMISE_BASE_H

// Cortado
//
#include <Cortado/Concepts/PreAndPostAction.h>
#include <Cortado/Concepts/TaskImpl.h>
#include <Cortado/Detail/AtomicRefCount.h>
#include <Cortado/Detail/CoroutineStorage.h>

namespace Cortado::Detail
{

struct None
{
};

template <typename T, bool FHasAdditionalStorage>
struct AdditionalStorageHelper
{
	using AdditionalStorageT = typename T::AdditionalStorage;
};

template <typename T>
struct AdditionalStorageHelper<T, false>
{
	using AdditionalStorageT = None;
};

template <Concepts::TaskImpl T, typename R>
struct CoroutinePromiseBase : AtomicRefCount<typename T::Atomic>
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
				return false;
			}

			bool await_suspend(std::coroutine_handle<>) noexcept
			{
				_this.BeforeSuspend();
				auto next = _this.m_storage.Continuation();
				if (next != nullptr)
				{
					next();
				}

				return _this.Release() > 0;
			}

			void await_resume() noexcept
			{
				// no _this.BeforeResume as the frame is expected to be destroyed
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

	bool SetContinuation(std::coroutine_handle<> h)
	{
		return m_storage.SetContinuation(h);
	}

	std::coroutine_handle<> GetContinuation()
	{
		return m_storage.Continuation();
	}

	void BeforeSuspend()
	{
		if constexpr (Concepts::HasAdditionalStorage<T>)
		{
			T::OnBeforeSuspend(m_additionalStorage);
		}
	}

	void BeforeResume()
	{
		if constexpr (Concepts::HasAdditionalStorage<T>)
		{
			T::OnBeforeResume(m_additionalStorage);
		}
	}

protected:
	using AdditionalStorageT =
		AdditionalStorageHelper<T, Concepts::HasAdditionalStorage<T>>::AdditionalStorageT;

	CoroutineStorage<R, typename T::Exception, typename T::Atomic> m_storage;
	[[no_unique_address]] AdditionalStorageT m_additionalStorage;

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
