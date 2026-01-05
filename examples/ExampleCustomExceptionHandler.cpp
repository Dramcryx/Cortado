/// @file ExampleCustomExceptionHandler.cpp
/// Example of customizing an exception handler.
///

// Cortado
//
#include <Cortado/Await.h>

// STL
//
#include <cassert>
#include <cstdio>
#include <stdexcept>

#ifndef _WIN32
#define E_UNEXPECTED                     0x8000FFFFL
#define E_OUTOFMEMORY                    0x80000002L
#define E_INVALIDARG                     0x80000003L
#define E_FAIL                           0x80000008L
#endif // !_WIN32

// Pretend we throw HRESULTs instead of exception.
//
class CustomExceptionHandler
{
public:
	using Exception = long;

	inline static Exception Catch()
	{
		try
		{
			throw; // rethrow current exception
		}
		catch (const std::invalid_argument &)
		{
            return E_INVALIDARG;
		}
        catch (const std::bad_alloc &)
		{
			return E_OUTOFMEMORY;
		}
		catch (const std::runtime_error&)
        {
			return E_FAIL;
		}
		catch (...)
		{
            return E_UNEXPECTED;
		}
	}

	inline static void Rethrow(Exception &&ex)
	{
		throw ex;
    }
};

struct CustomTaskImpl :
	Cortado::Common::STLAtomic,
	Cortado::Common::STLCoroutineAllocator,
	Cortado::DefaultScheduler,
	CustomExceptionHandler
{
    using Event = Cortado::DefaultEvent;
};

Cortado::Task<void, CustomTaskImpl> ExampleFunctionThatThrows()
{
	co_await Cortado::ResumeBackground();
	throw std::runtime_error("Example error");
}

int main()
{
	try
	{
		ExampleFunctionThatThrows().Get();
	}
	catch (long errCode)
	{
        assert(errCode == E_FAIL);
		printf("Caught error code: 0x%lX\n", errCode);
	}

	return 0;
}
