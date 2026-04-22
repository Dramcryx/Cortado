/// @file AwaiterBase.h
/// Base class for all awaiters in Cortado
///

#ifndef CORTADO_AWAITER_BASE_H
#define CORTADO_AWAITER_BASE_H

// Cortado
//
#include <Cortado/Detail/PromiseType.h>

namespace Cortado
{
/// @brief Core awaiter struct. Reusable code for user storage
/// and async stack tracing.
///
struct AwaiterBase
{
protected:
    using Base = AwaiterBase;

    /// @brief Actions before suspend. Handles user storage and
    /// async stack tracing hooks.
    ///
    template <Concepts::TaskImpl T, typename R>
    inline void await_suspend(
        std::coroutine_handle<Detail::PromiseType<T, R>> h)
    {
        if constexpr (Concepts::HasAdditionalStorage<T> ||
                      Concepts::AsyncStackTracing<T>)
        {
            m_handle = h;
            m_beforeResumeFunc = BeforeResumeFunc<T, R>;
            h.promise().BeforeSuspend();
        }
    }

    /// @brief Actions before resumption. Handles user storage and
    /// async stack tracing hooks.
    ///
    inline void await_resume()
    {
        if (m_handle)
        {
            m_beforeResumeFunc(m_handle);
        }
    }

private:
    /// @brief Type-eraser for awaiters to call before resumption.
    ///
    using BeforeResumeFuncT = void (*)(std::coroutine_handle<>);

    std::coroutine_handle<> m_handle{nullptr};
    BeforeResumeFuncT m_beforeResumeFunc{nullptr};

    /// @brief Type-erased function for awaiters to call before resumption.
    /// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
    /// @tparam R Return type of coroutine.
    ///
    template <Concepts::TaskImpl T, typename R>
    static void BeforeResumeFunc(std::coroutine_handle<> h)
    {
        if constexpr (Concepts::HasAdditionalStorage<T> ||
                      Concepts::AsyncStackTracing<T>)
        {
            auto restored =
                std::coroutine_handle<Detail::PromiseType<T, R>>::from_address(
                    h.address());
            restored.promise().BeforeResume();
        }
    }
};
} // namespace Cortado

#endif // CORTADO_AWAITER_BASE_H
