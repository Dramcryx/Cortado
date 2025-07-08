#ifndef CORTADO_TASK_H
#define CORTADO_TASK_H

// Cortado
//
#include <Cortado/DefaultTaskImpl.h>
#include <Cortado/Detail/CoroutinePromiseBase.h>

// STL
//
#include <utility>

namespace Cortado
{

template <typename R, Concepts::TaskImpl T>
class Task;

template <Concepts::TaskImpl T, typename R>
struct PromiseType : Detail::CoroutinePromiseBaseWithValue<T, R>
{
	using Allocator = typename T::Allocator;

	PromiseType()
	{
		static_assert(std::is_default_constructible_v<Allocator>);
	}

	template <typename ... TArgs>
	PromiseType(TArgs&& ... args) :
		PromiseType{ AllocatorSearchTag{}, std::forward<TArgs>(args)... }
	{
	}

	static void* operator new(std::size_t size, Allocator a)
	{
		return a.allocate(size);
	}

	static void* operator new(std::size_t size)
	{
		return operator new(size, Allocator{});
	}

	Task<R, T> get_return_object();

private:
	Allocator m_alloc;

	struct AllocatorSearchTag {};

	template <typename ... TArgs>
	PromiseType(AllocatorSearchTag, Allocator al, TArgs&& ... args) :
		m_alloc{ al }
	{
	}

	template <typename ... TArgs>
	PromiseType(AllocatorSearchTag, TArgs&& ... args)
	{
		static_assert(std::is_default_constructible_v<Allocator>);
	}
};

template <typename R = void, Concepts::TaskImpl T = DefaultTaskImpl>
class Task
{
public:
	using promise_type = PromiseType<T, R>;

	Task(std::coroutine_handle<PromiseType<T, R>> h) :
		m_handle{ h }
	{
	}

	~Task()
	{
		if (m_handle.promise().Release() == 0)
		{
			m_handle.destroy();
		}
	}

	decltype(auto) Get()
	{
		for (; !m_handle.promise().Ready(););
			
		return m_handle.promise().Get();
	}

	bool await_ready()
	{
		return m_handle.promise().Ready();
	}

	template <Concepts::TaskImpl T2, typename R2>
	void await_suspend(std::coroutine_handle<PromiseType<T2, R2>> h)
	{
		m_handle.promise().SetContinuation(h);
	}

	decltype(auto) await_resume()
	{
		return Get();
	}

private:
	std::coroutine_handle<PromiseType<T, R>> m_handle;
};

template <Concepts::TaskImpl T, typename R>
Task<R, T> PromiseType<T, R>::get_return_object()
{
	this->AddRef();
	return Task<R, T>{std::coroutine_handle<PromiseType<T, R>>::from_promise(*this)};
}

} // namespace Cortado

#endif
