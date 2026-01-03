/// @file AwaiterBase.h
/// Base class for all awaiters in Cortado
///

#ifndef CORTADO_AWAITER_BASE_H
#define CORTADO_AWAITER_BASE_H

// Cortado
//
#include <Cortado/Task.h>

namespace Cortado
{
namespace Detail
{
/// @brief Type-eraser for awaiters to call before resumption.
///
using BeforeResumeFuncT = void (*)(std::coroutine_handle<>);

/// @brief Type-erased function for awairers to call before resumption.
/// @tparam T @link Cortado::Concepts::TaskImpl TaskImpl@endlink.
/// @tparam R Return type of coroutine.
///
template <Concepts::TaskImpl T, typename R>
void BeforeResumeFunc(std::coroutine_handle<> h)
{
    if constexpr (Concepts::HasAdditionalStorage<T>)
    {
        auto restored =
            std::coroutine_handle<Cortado::PromiseType<T, R>>::from_address(
                h.address());
        restored.promise().BeforeResume();
    }
}
} // namespace Detail

/// @brief Core awaiter struct. Reusable code for user storage.
///
struct AwaiterBase
{
protected:
    using Base = AwaiterBase;

    /// @brief Actions before suspend. If no user storage is defined,
    /// this method is no-op.
    ///
    template <Concepts::TaskImpl T, typename R>
    inline void await_suspend(
        std::coroutine_handle<Cortado::PromiseType<T, R>> h)
    {
        if constexpr (Concepts::HasAdditionalStorage<T>)
        {
            m_handle = h;
            m_beforeResumeFunc = Detail::BeforeResumeFunc<T, R>;
            h.promise().BeforeSuspend();
        }
    }

    /// @brief Actions before resumption. If no user storage is defined,
    /// this method is no-op.
    ///
    inline void await_resume()
    {
        if (m_handle)
        {
            m_beforeResumeFunc(m_handle);
        }
    }

private:
    std::coroutine_handle<> m_handle{nullptr};
    Detail::BeforeResumeFuncT m_beforeResumeFunc{nullptr};
};
} // namespace Cortado

#endif // CORTADO_AWAITER_BASE_H
