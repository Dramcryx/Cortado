# Cortado - a C++ coroutines strategy

# Goals
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

The goal of this strategy is to provide a basic `Task` and set of common awaiters, which you can customize exception behavior, atomic implementation, and runtime implementation for basic things such as offloading coroutine to background (like `co_await _winrt::resume_background()`);

# TODO
1) Support custom allocators;
2) Extend cotract with async mutexes and event - this would allow to implement new awaiters;
3) Introduce more system-specific atomics and runtimes - macOS libkern, more Win32, try doing Linux as well;
4) Cover with docs;
5) Standartize format.
