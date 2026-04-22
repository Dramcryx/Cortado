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
/// an opaque pointer (async stack frame) per thread.
/// @tparam T Candidate TLS provider type.
///
template <typename T>
concept AsyncStackTLS = requires(void *p) {
    { T::Get() } -> std::convertible_to<void *>;
    { T::Set(p) };
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
