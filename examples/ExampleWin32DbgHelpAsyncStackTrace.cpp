/// @file ExampleWin32DbgHelpAsyncStackTrace.cpp
/// Example of integrating Cortado's async stack tracing with DbgHelp on
/// Win32 to produce diagnostic dumps containing both the native call
/// stack and the logical async coroutine chain.
///
/// Each AsyncStackFrame carries a returnAddress captured at coroutine
/// creation; symbolizing it with SymFromAddr resolves to the user
/// coroutine's name. No annotation map, no macros, no per-coroutine
/// boilerplate.
///

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Common/Win32AsyncStackTLS.h>
#include <Cortado/DefaultTaskImpl.h>
#include <Cortado/Detail/AsyncStackFrame.h>

// Win32 / DbgHelp
//
#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

// STL
//
#include <iostream>
#include <string>

/// @brief A TaskImpl with async stack tracing enabled (Win32 TLS provider).
///
struct TaskImplWithAsyncStack :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLCoroutineAllocator,
    Cortado::Common::STLExceptionHandler,
    Cortado::DefaultScheduler
{
    using Event = Cortado::DefaultEvent;
    using AsyncStackTLSProvider = Cortado::Common::Win32AsyncStackTLS;
};

template <typename T = void>
using Task = Cortado::Task<T, TaskImplWithAsyncStack>;

using AsyncFrame =
    Cortado::Detail::AsyncStackFrame<Cortado::Common::Win32AsyncStackTLS>;

// ---------------------------------------------------------------------------
// DbgHelp symbolization
// ---------------------------------------------------------------------------

/// @brief Resolve a code address to "Symbol+offset (file:line)".
///
static std::string SymbolizeAddress(void *address)
{
    if (address == nullptr)
    {
        return "<null>";
    }

    HANDLE process = GetCurrentProcess();

    // SYMBOL_INFO has a flexible-array Name field; allocate enough storage.
    //
    alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    auto *symbol = reinterpret_cast<SYMBOL_INFO *>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    auto addr64 = reinterpret_cast<DWORD64>(address);

    if (!SymFromAddr(process, addr64, &displacement, symbol))
    {
        return "<no symbol>";
    }

    std::string result = symbol->Name;
    char hex[32];
    sprintf_s(hex, "+0x%llx", displacement);
    result += hex;

    // Optionally append source file/line if available.
    // MSVC emits sentinel line numbers (e.g. 0xF00000) for synthesized
    // coroutine ramp/$_Resume thunks that have no real source mapping;
    // skip those to avoid printing nonsense.
    //
    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD lineDisp = 0;
    if (SymGetLineFromAddr64(process, addr64, &lineDisp, &line))
    {
        constexpr DWORD kMaxRealLine = 1'000'000;
        char loc[512];
        if (line.LineNumber < kMaxRealLine)
        {
            sprintf_s(loc, " (%s:%lu)", line.FileName, line.LineNumber);
        }
        else
        {
            sprintf_s(loc, " (%s)", line.FileName);
        }
        result += loc;
    }

    return result;
}

// ---------------------------------------------------------------------------
// Combined dump
// ---------------------------------------------------------------------------

/// @brief Print both the native sync stack and the symbolized async chain.
///
static void DumpStacks(const char *label)
{
    std::cout << "================ " << label << " ================\n";

    // Native call stack
    //
    std::cout << "Native call stack:\n";
    void *stack[64];
    USHORT frames =
        RtlCaptureStackBackTrace(0, ARRAYSIZE(stack), stack, nullptr);
    for (USHORT i = 0; i < frames; ++i)
    {
        std::cout << "  [" << i << "] "
                  << SymbolizeAddress(stack[i]) << "\n";
    }

    // Async coroutine chain
    //
    std::cout << "Async coroutine chain (top -> root):\n";
    int depth = 0;
    Cortado::Detail::WalkCurrentAsyncStack<
        Cortado::Common::Win32AsyncStackTLS>(
        [&](const AsyncFrame &frame) -> bool
        {
            std::cout << "  [" << depth++ << "] "
                      << SymbolizeAddress(frame.returnAddress) << "\n";
            return true;
        });
    if (depth == 0)
    {
        std::cout << "  (empty)\n";
    }

    std::cout << std::endl;
}

// ---------------------------------------------------------------------------
// Demo coroutines
// ---------------------------------------------------------------------------

Task<int> GrandChild()
{
    DumpStacks("Inside GrandChild");
    co_await Cortado::ResumeBackground();
    DumpStacks("Inside GrandChild (after ResumeBackground)");
    co_return 42;
}

Task<int> Child()
{
    co_return co_await GrandChild();
}

Task<int> Parent()
{
    co_return co_await Child();
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main()
{
    HANDLE process = GetCurrentProcess();

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, nullptr, TRUE))
    {
        std::cerr << "SymInitialize failed: " << GetLastError() << "\n";
        return 1;
    }

    int result = Parent().Get();
    std::cout << "Result: " << result << "\n";

    SymCleanup(process);
    return 0;
}
