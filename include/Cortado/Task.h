/// @file Task.h
/// Task implementation.
///

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

/// @brief Final promise type.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @tparam R Return type of coroutine.
///
template <Concepts::TaskImpl T, typename R>
struct PromiseType : Detail::CoroutinePromiseBaseWithValue<T, R>
{
    using Allocator = typename T::Allocator;

    /// @brief Default constructor.
    ///
    PromiseType() = default;

    /// @brief Compiler contract: Defining `new` that uses custom allocator.
    ///
    template <typename... Args>
    static void *operator new(std::size_t size, Allocator a, Args &...)
    {
        return a.allocate(size);
    }

    /// @brief Compiler contract: Defining `new` that uses custom allocator for
    /// methods.
    ///
    template <typename Class, typename... Args>
    static void *operator new(std::size_t size, Class &, Allocator a, Args &...)
        requires(
            !std::convertible_to<std::remove_cvref_t<Class> &, Allocator &>)
    {
        return operator new(size, a);
    }

    /// @brief Compiler contract: Defining `new` that uses custom allocator
    /// which can be default-constructed.
    ///
    static void *operator new(std::size_t size)
        requires std::is_default_constructible_v<Allocator>
    {
        return operator new(size, Allocator{});
    }

    /// @brief Compiler contract: First suspension point return value.
    /// @returns Task object.
    ///
    Task<R, T> get_return_object();
};

/// @brief Task implementation type.
/// @tparam R Return type of coroutine.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
///
template <typename R = void, Concepts::TaskImpl T = DefaultTaskImpl>
class Task
{
public:
    struct TaskAwaiter;
    struct TaskLValueAwaier;

    using promise_type = PromiseType<T, R>;

    /// @brief Constructor.
    /// @param h Coroutine handle.
    ///
    Task(std::coroutine_handle<PromiseType<T, R>> h) : m_handle{h}
    {
    }

    /// @brief Move constructor.
    ///
    Task(Task &&other) noexcept
    {
        Reset();
        m_handle = std::exchange(other.m_handle, nullptr);
    }

    /// @brief Move assignment.
    ///
    Task &operator=(Task &&other) noexcept
    {
        Reset();
        m_handle = std::exchange(other.m_handle, nullptr);

        return *this;
    }

    /// @brief Non-copyable.
    ///
    Task(const Task &) = delete;

    /// @brief Non-copyable.
    ///
    Task &operator=(const Task &) = delete;

    /// @brief Destructor. Decrements reference count on the promise
    /// and destroys it if needed.
    ///
    ~Task()
    {
        Reset();
    }

    /// @brief Test if task is completed.
    /// @returns true is task is completed, false otherwise.
    ///
    inline bool IsReady() const
    {
        return m_handle.promise().Ready();
    }

    /// @brief Wait task completion for indefinite amout of time.
    ///
    inline void Wait()
    {
        m_handle.promise().Wait();
    }

    /// @brief Wait for task completion in a period of time.
    /// @param timeToWaitMs How many milliseconds to wait.
    /// @returns true if event was set in timeToWaitMs, false otherwise.
    ///
    inline bool WaitFor(unsigned long timeToWaitMs)
    {
        return m_handle.promise().WaitFor(timeToWaitMs);
    }

    /// @brief Get task result.
    /// @returns Task result.
    /// @throws Exception if present.
    ///
    decltype(auto) Get()
    {
        m_handle.promise().Wait();

        return m_handle.promise().Get();
    }

private:
    std::coroutine_handle<PromiseType<T, R>> m_handle;

    /// @brief Lifetime helper. If promise reference count reaches zero,
    /// the promise is destroyed.
    ///
    void Reset()
    {
        if (m_handle && m_handle.promise().Release() == 0)
        {
            m_handle.destroy();
        }
    }
};

/// @brief Compiler contract: First suspension point return value.
/// @returns Task object.
///
template <Concepts::TaskImpl T, typename R>
Task<R, T> PromiseType<T, R>::get_return_object()
{
    this->AddRef();
    return Task<R, T>{
        std::coroutine_handle<PromiseType<T, R>>::from_promise(*this)};
}

} // namespace Cortado

#endif
