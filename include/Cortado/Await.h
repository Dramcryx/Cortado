#ifndef CORTADO_AWAIT_H
#define CORTADO_AWAIT_H

// Cortado
//
#include <Cortado/Concepts/BackgroundResumable.h>
#include <Cortado/Task.h>

namespace Cortado
{

struct ResumeBackgroundAwaiter
{
	bool await_ready()
	{
		return false;
	}

	template <Concepts::BackgroundResumable T, typename R>
	void await_suspend(std::coroutine_handle<Cortado::PromiseType<T, R>> h)
	{
		T::GetDefaultBackgroundScheduler().Schedule(h);
	}

	void await_resume()
	{
	}
};

inline ResumeBackgroundAwaiter ResumeBackground()
{
	return {};
}

template <Concepts::TaskImpl T, typename R, typename ... Args>
Task<T, void> WhenAll(Task<T, R>& first, Args ... next)
{
	co_await std::forward<Task<T,R>>(first);
	(void(co_await next), ...);
}

} // namespace Cortado

#endif