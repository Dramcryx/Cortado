/// @file DefautlAsyncStackTLS.h
/// A default TLS selector
///

#ifndef CORTADO_DEFAULT_ASYNC_STACK_TLS_H
#define CORTADO_DEFAULT_ASYNC_STACK_TLS_H

#ifdef _WIN32

#include <Cortado/Common/Win32AsyncStackTLS.h>

namespace Cortado
{
using DefaultAsyncStackTLS = Common::Win32AsyncStackTLS;
} // namespace Cortado

#else // !_WIN32

#include <Cortado/Common/ClangOrGccAsyncStackTLS.h>

namespace Cortado
{
using DefaultAsyncStackTLS = Common::ClangOrGccAsyncStackTLS;
} // namespace Cortado

#endif // _WIN32


#endif // CORTADO_DEFAULT_ASYNC_STACK_TLS_H
