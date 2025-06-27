#ifndef CORTADO_TASK_H
#define CORTADO_TASK_H

// Cortado
//
#include <Cortado/Detail/CoroutinePromiseBase.h>

namespace Cortado
{

template <Concepts::TaskImpl T, typename R>
class Task;

template <Concepts::TaskImpl T, typename R>
struct PromiseType : Detail::CoroutinePromiseBaseWithValue<T, R>
{
	Task<T, R> get_return_object();
};

template <Concepts::TaskImpl T, typename R = void>
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
Task<T, R> PromiseType<T, R>::get_return_object()
{
	this->AddRef();
	return Task<T, R>{std::coroutine_handle<PromiseType<T, R>>::from_promise(*this)};
}

} // namespace Cortado

#endif
