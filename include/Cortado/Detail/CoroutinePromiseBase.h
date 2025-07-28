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

enum class CallbackRaceState : unsigned long
{
    None = 0,
    Value,
    Callback
};

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
                _this.m_completionEvent.Set();
                _this.CallbackValueRendezvous();

                auto next = _this.Continuation();
                if (next != nullptr)
                {
                    next();
                }

                return _this.Release() > 0;
            }

            void await_resume() noexcept
            {
                // no _this.BeforeResume as the frame is expected to be
                // destroyed
            }

            CoroutinePromiseBase &_this;
        };

        return FinalAwaiter{*this};
    }

    void unhandled_exception()
    {
        m_storage.SetError(T::Catch());
    }

    bool Ready()
    {
        return m_completionEvent.IsSet();
    }

    void Wait()
    {
        m_completionEvent.Wait();
    }

    bool WaitFor(unsigned long timeToWaitMs)
    {
        return m_completionEvent.WaitFor(timeToWaitMs);
    }

    bool SetContinuation(std::coroutine_handle<> h)
    {
        m_continuation = h;
        CallbackRaceState expectedState = CallbackRaceState::None;
        if (m_callbackRace.compare_exchange_strong(
                *reinterpret_cast<unsigned long *>(&expectedState),
                static_cast<unsigned long>(CallbackRaceState::Callback)))
        {
            // Successfully stored callback first

            return true;
        }
        else if (expectedState == CallbackRaceState::Value)
        {
            m_continuation();
            m_continuation = nullptr;
        }

        return false;
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

    void CallbackValueRendezvous()
    {
        CallbackRaceState expectedState = CallbackRaceState::None;
        if (m_callbackRace.compare_exchange_strong(
                *reinterpret_cast<unsigned long *>(&expectedState),
                static_cast<unsigned long>(CallbackRaceState::Value)))
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

protected:
    using ExceptionT = typename T::Exception;
    using AtomicT = typename T::Atomic;
    using EventT = typename T::Event;
    using AdditionalStorageT =
        AdditionalStorageHelper<T, Concepts::HasAdditionalStorage<T>>::
            AdditionalStorageT;

    // Essential storage - stores value or exception
    CoroutineStorage<R, ExceptionT, AtomicT> m_storage;

    // Race flag between possible continuation and coroutine
    AtomicT m_callbackRace{static_cast<unsigned long>(CallbackRaceState::None)};

    // Completion flag
    EventT m_completionEvent;

    // Continuation - the coroutine that awaits for this one, if any
    std::coroutine_handle<> m_continuation = nullptr;

    // Optional user storage
    [[no_unique_address]] AdditionalStorageT m_additionalStorage;

    void RethrowError()
    {
        if (m_storage.GetHeldValueType() == HeldValue::Error)
        {
            T::Rethrow(std::move(m_storage.UnsafeError()));
        }
    }

    std::coroutine_handle<> &Continuation()
    {
        return m_continuation;
    }
};

template <Concepts::TaskImpl T, typename R>
struct CoroutinePromiseBaseWithValue : CoroutinePromiseBase<T, R>
{
    template <typename U>
    void return_value(U &&u)
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
