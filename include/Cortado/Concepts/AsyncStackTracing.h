/// @file AsyncStackTracing.h
/// Concepts for async stack tracing support.
///

#ifndef CORTADO_CONCEPTS_ASYNC_STACK_TRACING_H
#define CORTADO_CONCEPTS_ASYNC_STACK_TRACING_H

// STL
//
#include <concepts>

namespace Cortado::Concepts
{

/// @brief Concept for a TLS provider that can store and retrieve
/// an opaque pointer (async stack frame) per thread, and capture a
/// code address identifying the coroutine creation site.
///
/// CaptureReturnAddress() is invoked from inside initial_suspend(); it
/// is expected to return an address that a symbolizer (e.g. DbgHelp's
/// SymFromAddr) can resolve to the user coroutine. Implementations may
/// use a runtime stack walk (e.g. RtlCaptureStackBackTrace) and skip
/// the appropriate number of frames; returning nullptr is acceptable
/// when the platform cannot capture such an address.
/// @tparam T Candidate TLS provider type.
///
template <typename T>
concept AsyncStackTLS = requires(void *p) {
    { T::Get() } -> std::convertible_to<void *>;
    { T::Set(p) };
    { T::CaptureReturnAddress() } -> std::convertible_to<void *>;
};

/// @brief Concept for TaskImpl types that opt into async stack tracing.
/// The TaskImpl must define an AsyncStackTLSProvider type alias
/// that satisfies AsyncStackTLS.
/// @tparam T Candidate TaskImpl type.
///
template <typename T>
concept AsyncStackTracing = requires {
    typename T::AsyncStackTLSProvider;
    requires AsyncStackTLS<typename T::AsyncStackTLSProvider>;
};

} // namespace Cortado::Concepts

#endif // CORTADO_CONCEPTS_ASYNC_STACK_TRACING_H
