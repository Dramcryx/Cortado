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
    /// We will allocate extra space before the aligned frame to store the
    /// allocator instance which will be used during deallocation.
    ///
    /// @tparam Alloc This one has to be `Allocator`. A template is used here
    /// only to enable universal reference. The actual allocator instance is
    /// always copied inside `AllocateFrame`.
    ///
    /// @tparam Args Other coroutine arguments - ignored.
    ///
    /// @param size `new` operator contract. Frame size calculated by compiler.
    /// @param a Allocator to use for frame allocation and deallocation.
    ///
    /// @returns Pointer to coroutine frame start.
    ///
    template <Concepts::CoroutineAllocator Alloc, typename... Args>
    [[nodiscard]] static void *operator new(std::size_t size,
                                            Alloc &&a,
                                            [[maybe_unused]] Args &&...)
        requires(std::is_same_v<std::remove_cvref_t<Alloc>, Allocator>)
    {
        return AllocateFrame(size, a);
    }

    /// @brief Compiler contract: Defining `new` that uses custom allocator.
    /// We will allocate extra space before the aligned frame to store the
    /// allocator instance which will be used during deallocation.
    /// This specific overload is designated to class methods (or lambdas).
    ///
    /// @tparam Class A class for which consturcted coroutine is a method.
    ///
    /// @tparam Alloc This one has to be `Allocator`. A template is used here
    /// only to enable universal reference. The actual allocator instance is
    /// always copied inside `AllocateFrame`.
    ///
    /// @tparam Args Other coroutine arguments - ignored.
    ///
    /// @param size `new` operator contract. Frame size calculated by compiler.
    /// @param a Allocator to use for frame allocation and deallocation.
    ///
    /// @returns Pointer to coroutine frame start.
    ///
    template <typename Class,
              Concepts::CoroutineAllocator Alloc,
              typename... Args>
    [[nodiscard]] static void *operator new(std::size_t size,
                                            [[maybe_unused]] Class &,
                                            Alloc &&a,
                                            [[maybe_unused]] Args &&...)
        requires(
            !std::convertible_to<std::remove_cvref_t<Class> &, Allocator &> &&
            std::is_same_v<std::remove_cvref_t<Alloc>, Allocator>)
    {
        return AllocateFrame(size, a);
    }

    /// @brief Compiler contract: Defining `new` that uses custom allocator.
    /// We will allocate extra space before the aligned frame to store the
    /// allocator instance which will be used during deallocation.
    /// This specific overload is only supported if allocator is not provided
    /// within coroutine arguments list and if allocator is
    /// default-constructible (such as std::allocator).
    ///
    /// @param size `new` operator contract. Frame size calculated by compiler.
    ///
    /// @returns Pointer to coroutine frame start.
    ///
    [[nodiscard]] static void *operator new(std::size_t size)
        requires std::is_default_constructible_v<Allocator>
    {
        Allocator a;
        return AllocateFrame(size, a);
    }

    /// @brief Compiler contract: Defining `delete` that uses custom allocator.
    ///
    /// @param ptr Pointer to coroutine frame start.
    /// @param size Frame size calculated by compiler.
    ///
    static void operator delete(void *ptr, std::size_t size) noexcept
    {
        std::byte *rawMemory = static_cast<std::byte *>(ptr);

        // Offset back to allocator storage
        //
        rawMemory -= AllocatorSizeAlignedByPromise();

        // Cast to allocator pointer
        //
        Allocator *allocatorPtr = reinterpret_cast<Allocator *>(rawMemory);

        // Extract allocator from storage
        //
        Allocator allocator{
            [allocatorPtr]()
            {
                if constexpr (std::is_move_constructible_v<Allocator>)
                {
                    return std::move(*allocatorPtr);
                }
                else
                {
                    return *allocatorPtr;
                }
            }()};

        // Destroy in-frame stored allocator as we are going to clear out the
        // frame
        //
        allocatorPtr->~Allocator();

        // Restore total allocation size
        //
        const std::size_t totalAllocationSize =
            AllocatorSizeAlignedByPromise() + size;

        allocator.deallocate(rawMemory, totalAllocationSize);
    }

    /// @brief Compiler contract: First suspension point return value.
    /// @returns Task object.
    ///
    Task<R, T> get_return_object();

    /// @brief Compiler contract: Core checkpoint for all awaiters
    /// @returns Awaitable that is returned by respective `co_await` operator.
    ///
    template <typename U>
    U &&await_transform(U &&awaitable) noexcept
    {
        return static_cast<U &&>(awaitable);
    }

private:
    /// @brief Helper for frame allocation.
    /// @param size Size of aligned frame requested by compiler.
    /// @param a Allocator instance to use for allocation.
    /// @returns Pointer to aligned frame start.
    ///
    inline static void *AllocateFrame(std::size_t size, Allocator &a)
    {
        // Total allocation size includes:
        // --------------------------------------
        // | Allocator instance | Aligned frame |
        // --------------------------------------
        //
        const std::size_t totalAllocationSize =
            AllocatorSizeAlignedByPromise() + size;

        // Allocate full storage size
        //
        void *rawPtr = a.allocate(totalAllocationSize);

        if (rawPtr == nullptr)
        {
            return nullptr;
        }

        // Copy-place the allocator
        //
        ::new (rawPtr) Allocator{a};

        // Advance pointer to 'after allocator' position and return
        //
        std::byte *rawMemory = static_cast<std::byte *>(rawPtr);

        return rawMemory + AllocatorSizeAlignedByPromise();
    }

    /// @brief Helper for alignment calculations.
    /// Canonical alignment formula to ensure allocator is properly aligned
    /// @returns Aligned size of Allocator.
    ///
    inline static constexpr std::size_t AllocatorSizeAlignedByPromise()
    {
        constexpr std::size_t frameAlignment = alignof(PromiseType);
        constexpr std::size_t allocatorAlignment = alignof(Allocator);

        constexpr std::size_t finalAllocatorAlignment =
            allocatorAlignment > frameAlignment ? allocatorAlignment
                                                : frameAlignment;

        return (sizeof(Allocator) + (finalAllocatorAlignment - 1)) &
               ~(finalAllocatorAlignment - 1);
    }
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
