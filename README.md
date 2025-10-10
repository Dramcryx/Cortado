# Cortado - a C++ coroutines strategy

# Introduction
[C++ coroutines do not spark joy](https://probablydance.com/2021/10/31/c-coroutines-do-not-spark-joy/) and that is for multiple reasons:
1) No library support - you are only provided with compiler contract on top of which you build your own library;
2) No basic runtime support - use `boost::asio`, `Win32`, custom threadpools etc.;
3) No stack trace support - also build your own.

Even if we take all these into account, there are many similarities found across existing implementations:
1) They return value or rethrow exception in a similar manner;
2) They adress continuations (i.e. `co_await otherTask()`) in a similar manner;
3) They introduce similar awaiters - to wait any, to wait all, resume when all etc.

According to my personal experience, there are additional limitations for older projects which you want to modernize using coroutines:
1) they might rely on custom allocators - I cannot use coroutines with default `new`;
2) they might rely on customized or extended exception handling:
    - if they don't link STL runtime at all, you cannot use `std::exception_ptr` (unless you copy-paste ABI code which is a bad practice);
	- if they do but deny raw `throw`/`catch` when handling errors, we also cannot use `std::exception_ptr` because all additional exception setup is lost when rethrowing an exception.

# Goal

The goal of this strategy is to provide a basic `Task` and set of common awaiters, which you can customize exception behavior, atomic implementation, and runtime implementation for basic things such as offloading coroutine to background (like `co_await winrt::resume_background()`), awaiting another coroutine or a group of them. This is something that I would expect STL to offer but the current direction of commitee is towards `std::execution` rather than user-defined.

# Non-goal

Performance tinkering. While this library aims to write optimal code following GSL practices, there is no space for low-level optimizations as they usually involve platform-specific work (both OS and hardware), which makes it too complex to maintain Cortado as platform-independent as possible.

# TODO
1) Implement cancellation.
2) Support fuzzer.
4) Support stack tracing - Natvis for cdb, scripts for gdb and lldb.
