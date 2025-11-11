/// @file CoroutinePromiseBase.h
/// Core data structure for coroutine promise.
///

#ifndef CORTADO_COROUTINE_PROMISE_BASE_H
#define CORTADO_COROUTINE_PROMISE_BASE_H

// Cortado
//
#include <Cortado/Concepts/PreAndPostAction.h>
#include <Cortado/Concepts/TaskImpl.h>
#include <Cortado/Detail/AtomicRefCount.h>
#include <Cortado/Detail/CoroutineStorage.h>

// STL
//
#include <coroutine>

namespace Cortado::Detail
{

/// @brief Helper class to define user-defined storage for coroutine.
/// @tparam T TaskImpl type.
/// @tparam FHasAdditionaStorage boolean inidicating if TaskImpl defines
/// custom user storage.
///
template <typename T, bool FHasAdditionalStorage>
struct AdditionalStorageHelper
{
    using AdditionalStorageT = typename T::AdditionalStorage;
};

/// @brief Helper class to define user-defined storage for coroutine.
/// @tparam T TaskImpl type.
//
template <typename T>
struct AdditionalStorageHelper<T, false>
{
    /// @brief Human-readable "no user storage" indicator.
    ///
    struct Nothing
    {
    };

    using AdditionalStorageT = Nothing;
};

/// @brief Core promise class. Encapsulates refcounting,
/// initial and final suspensions, exception handling and
/// continuation hadnling.
/// @tparam T A class that defines custom types needed for
/// coroutine strategy to function.
/// See more at @link Cortado::Concepts::TaskImpl TaskImpl@endlink
/// @tparam R Return value type.
///
template <Concepts::TaskImpl T, typename R>
struct CoroutinePromiseBase : AtomicRefCount<typename T::Atomic>
{
    /// @brief Compiler contract: Initial suspension.
    /// Never suspend at the beginning.
    ///
    std::suspend_never initial_suspend()
    {
        return {};
    }

    /// @brief Compiler contract: Final suspension.
    /// Do not suspend but call continuation if exists.
    ///
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
                // No _this.BeforeSuspend(); even if we suspend because
                // this frame is outside of coroutine function body.
                //
                _this.m_completionEvent.Set();
                auto next = std::coroutine_handle<>::from_address(_this.CallbackValueRendezvous());
                if (next != nullptr)
                {
                    next.resume();
                }

                return _this.Release() > 0;
            }

            void await_resume() noexcept
            {
                // no _this.BeforeResume(); as the frame is expected to be
                // destroyed
            }

            CoroutinePromiseBase &_this;
        };

        return FinalAwaiter{*this};
    }

    /// @brief Compiler contract: Actions on unhandled exception.
    /// Call user-defined cathcer.
    ///
    void unhandled_exception()
    {
        m_storage.SetError(T::Catch());
    }

    /// @brief Coroutine readiness flag.
    /// @returns true if coroutine has value/exception, false otherwise.
    ///
    bool Ready()
    {
        return m_completionEvent.IsSet();
    }

    /// @brief Wait completion event to be signaled.
    ///
    void Wait()
    {
        m_completionEvent.Wait();
    }

    /// @brief Wait for completion event for specified amount of time.
    /// @param timeToWaitMs Time to wait in milliseconds.
    ///
    bool WaitFor(unsigned long timeToWaitMs)
    {
        return m_completionEvent.WaitFor(timeToWaitMs);
    }

    /// @brief Set next coroutine to execute once this one is completed.
    /// @param h Handle to a coroutine which must be resumed once this
    /// coroutine is completed.
    /// @returns true if suspension of the current coroutine required, false
    /// otherwise.
    ///
    bool SetContinuation(std::coroutine_handle<> h)
    {
        auto expectedState = NoCompletionState;

        auto desiredState = reinterpret_cast<Concepts::AtomicPrimitive>(h.address());

        if (m_continuation.compare_exchange_strong(expectedState, desiredState))
        {
            // Successfully stored callback first
            return true;
        }
        else if (expectedState == ValueCompletionState)
        {
            h.resume();
        }

        return false;
    }

    /// @brief Call user-defined behavior over user-defined storage
    /// to perform specific actions before coroutine is suspended in the middle
    /// of execution.
    ///
    void BeforeSuspend()
    {
        if constexpr (Concepts::HasAdditionalStorage<T>)
        {
            T::OnBeforeSuspend(m_additionalStorage);
        }
    }

    /// @brief Call user-defined behavior over user-defined storage
    /// to perform specific actions before coroutine is resumed in the middle
    /// of execution.
    ///
    void BeforeResume()
    {
        if constexpr (Concepts::HasAdditionalStorage<T>)
        {
            T::OnBeforeResume(m_additionalStorage);
        }
    }

protected:
    using ExceptionT = typename T::Exception;
    using AtomicT = typename T::Atomic;
    using EventT = typename T::Event;

    static constexpr bool HasUserStroage = Concepts::HasAdditionalStorage<T>;

    using AdditionalStorageT =
        AdditionalStorageHelper<T, HasUserStroage>::AdditionalStorageT;

    static constexpr Concepts::AtomicPrimitive NoCompletionState = 0;
    static constexpr Concepts::AtomicPrimitive ValueCompletionState = 1;

    /// @brief Essential storage - stores value or exception.
    ///
    CoroutineStorage<R, ExceptionT, AtomicT> m_storage;

    /// @brief Completion event.
    ///
    EventT m_completionEvent;

    /// @brief Continuation - the coroutine that awaits for this one, if any.
    ///
    AtomicT m_continuation = 0;

    /// @brief Optional user storage.
    ///
    [[no_unique_address]] AdditionalStorageT m_additionalStorage;

    /// @brief Rethrows exception from result storage, if any.
    ///
    void RethrowError()
    {
        if (m_storage.GetHeldValueType() == HeldValue::Error)
        {
            T::Rethrow(std::move(m_storage.UnsafeError()));
        }
    }

private:
    /// @brief Handle race-condition between a thread that sets value and a
    /// thread that sets continuation. If value thread succeeds, we return nullptr.
    /// Otherwise we return pointer to continuation (i.e. continuation was set after
    /// coroutine completion).
    /// @returns Continuation or nullptr;
    ///
    void* CallbackValueRendezvous()
    {
        auto expectedState = NoCompletionState;

        auto desiredState = ValueCompletionState;

        if (m_continuation.compare_exchange_strong(expectedState, desiredState))
        {
            // Successfully stored value first
            return nullptr;
        }
        else if (expectedState > 1)
        {
            // Callback was already set, return to final_suspend
            return reinterpret_cast<void*>(expectedState);
        }

        return nullptr;
    }
};

/// @brief Core promise class with compiler contract required to write
/// `co_return value;`.
/// @tparam T A class that defines custom types needed for
/// coroutine strategy to function.
/// See more at @link Cortado::Concepts::TaskImpl TaskImpl@endlink
/// @tparam R Return value type.
///
template <Concepts::TaskImpl T, typename R>
struct CoroutinePromiseBaseWithValue : CoroutinePromiseBase<T, R>
{
    /// @brief Compiler contract: Returning a value from coroutine.
    /// @tparam U Type of return value that is equal to R or convertible to it.
    ///
    template <typename U>
    void return_value(U &&u)
    {
        this->m_storage.SetValue(std::forward<U>(u));
    }

    /// @brief Get stored value or rethrow exception.
    /// @returns Value from storage.
    /// @throws Exception from @link Cortado::Concepts::ErrorHandler
    /// ErrorHandler@endlink's `Rethrow` function.
    ///
    decltype(auto) Get()
    {
        this->RethrowError();
        return std::move(this->m_storage.UnsafeValue());
    }
};

/// @brief Core promise class with compiler contract required to write
/// `co_return;`.
/// @tparam T A class that defines custom types needed for
/// coroutine strategy to function.
/// See more at @link Cortado::Concepts::TaskImpl TaskImpl@endlink
/// @tparam R Return value type.
///
template <Concepts::TaskImpl T>
struct CoroutinePromiseBaseWithValue<T, void> : CoroutinePromiseBase<T, bool>
{
    /// @brief Compiler contract: Returning void from coroutine.
    ///
    void return_void()
    {
        this->m_storage.SetValue(true);
    }

    /// @brief Rethrow exception if any, do nothing oherwise.
    /// @throws Exception from @link Cortado::Concepts::ErrorHandler
    /// ErrorHandler@endlink's `Rethrow` function.
    ///
    void Get()
    {
        this->RethrowError();
    }
};

} // namespace Cortado::Detail

#endif
