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

    template <typename... TArgs>
    PromiseType(TArgs &&...args) :
        PromiseType{AllocatorSearchTag{}, std::forward<TArgs>(args)...}
    {
    }

    template <typename... Args>
    static void *operator new(std::size_t size, Allocator a, Args&...)
    {
        return a.allocate(size);
    }

    template <typename Class, typename... Args>
    static void *operator new(std::size_t size, Class &, Allocator a, Args&...)
        requires(
            !std::convertible_to<std::remove_cvref_t<Class> &, Allocator &>)
    {
        return operator new(size, a);
    }

    static void *operator new(std::size_t size)
    {
        return operator new(size, Allocator{});
    }

    Task<R, T> get_return_object();

private:
    Allocator m_alloc;

    struct AllocatorSearchTag
    {
    };

    template <typename... TArgs>
    PromiseType(AllocatorSearchTag, Allocator al, TArgs &&...args) : m_alloc{al}
    {
    }

    template <typename... TArgs>
    PromiseType(AllocatorSearchTag, TArgs &&...args)
    {
        static_assert(std::is_default_constructible_v<Allocator>);
    }
};

template <typename R = void, Concepts::TaskImpl T = DefaultTaskImpl>
class Task
{
public:
    struct TaskAwaiter;
    struct TaskLValueAwaier;

    using promise_type = PromiseType<T, R>;

    Task(std::coroutine_handle<PromiseType<T, R>> h) : m_handle{h}
    {
    }

    Task(Task &&other) noexcept
    {
        Reset();
        m_handle = std::exchange(other.m_handle, nullptr);
    }

    Task &operator=(Task &&other) noexcept
    {
        Reset();
        m_handle = std::exchange(other.m_handle, nullptr);

        return *this;
    }

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    ~Task()
    {
        Reset();
    }

    inline bool IsReady() const
    {
        return m_handle.promise().Ready();
    }

    inline void Wait()
    {
        m_handle.promise().Wait();
    }

    inline bool WaitFor(unsigned long timeToWaitMs)
    {
        return m_handle.promise().WaitFor(timeToWaitMs);
    }

    decltype(auto) Get()
    {
        m_handle.promise().Wait();

        return m_handle.promise().Get();
    }

private:
    std::coroutine_handle<PromiseType<T, R>> m_handle;

    void Reset()
    {
        if (m_handle && m_handle.promise().Release() == 0)
        {
            m_handle.destroy();
        }
    }
};

template <Concepts::TaskImpl T, typename R>
Task<R, T> PromiseType<T, R>::get_return_object()
{
    this->AddRef();
    return Task<R, T>{
        std::coroutine_handle<PromiseType<T, R>>::from_promise(*this)};
}

} // namespace Cortado

#endif
