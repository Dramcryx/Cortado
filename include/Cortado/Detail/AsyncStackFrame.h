/// @file AsyncStackFrame.h
/// Async stack frame for coroutine stack tracing.
///

#ifndef CORTADO_DETAIL_ASYNC_STACK_FRAME_H
#define CORTADO_DETAIL_ASYNC_STACK_FRAME_H

// Cortado
//
#include <Cortado/Concepts/AsyncStackTracing.h>

namespace Cortado::Detail
{

/// @brief A single frame in the async coroutine call stack.
/// Templated on a TLS provider so that the thread-local storage
/// mechanism can be injected by the user.
/// @tparam TLS A type satisfying Concepts::AsyncStackTLS.
///
template <Concepts::AsyncStackTLS TLS>
struct AsyncStackFrame
{
    /// @brief Pointer to the parent (caller) frame in the async stack.
    ///
    AsyncStackFrame *parentFrame{nullptr};

    /// @brief Code address captured when the coroutine was created.
    /// Points into the user coroutine's bootstrap code (the function
    /// that called the coroutine factory); resolvable by symbolizers
    /// such as DbgHelp's SymFromAddr to identify the coroutine.
    /// May be nullptr if not captured.
    ///
    void *returnAddress{nullptr};

    /// @brief Get the currently active async stack frame for this thread.
    /// @returns Pointer to the top frame, or nullptr if none.
    ///
    static AsyncStackFrame *GetCurrent()
    {
        return static_cast<AsyncStackFrame *>(TLS::Get());
    }

    /// @brief Set the currently active async stack frame for this thread.
    /// @param frame The frame to set as current, or nullptr.
    ///
    static void SetCurrent(AsyncStackFrame *frame)
    {
        TLS::Set(frame);
    }
};

/// @brief Walk the async stack starting from a given frame.
/// @tparam TLS The TLS provider type.
/// @tparam Fn Callback type — signature: bool(const AsyncStackFrame<TLS>&).
///           Return false to stop walking.
/// @param frame Starting frame (may be nullptr).
/// @param callback Invoked for each frame.
///
template <Concepts::AsyncStackTLS TLS, typename Fn>
inline void WalkAsyncStackFrom(AsyncStackFrame<TLS> *frame, Fn &&callback)
{
    while (frame)
    {
        if (!callback(*frame))
        {
            return;
        }

        frame = frame->parentFrame;
    }
}

/// @brief Walk the async stack starting from the current thread's top frame.
/// @tparam TLS The TLS provider type.
/// @tparam Fn Callback type — signature: bool(const AsyncStackFrame<TLS>&).
///           Return false to stop walking.
/// @param callback Invoked for each frame.
///
template <Concepts::AsyncStackTLS TLS, typename Fn>
inline void WalkCurrentAsyncStack(Fn &&callback)
{
    WalkAsyncStackFrom(AsyncStackFrame<TLS>::GetCurrent(), callback);
}

} // namespace Cortado::Detail

#endif // CORTADO_DETAIL_ASYNC_STACK_FRAME_H
