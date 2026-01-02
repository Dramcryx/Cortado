/// @file CoroutineAllocatorTests.cpp
/// Tests for allocator behavior.
///

// gtest
//
#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/Await.h>

// STL
//
#include <atomic>
#include <unordered_set>

struct SharedAllocatorState
{
    std::atomic_int AllocationCount = 0;

    std::atomic_int DeallocationCount = 0;

    std::atomic_int CopyConstructedCount = 0;

    std::atomic_int MoveConstructedCount = 0;

    std::atomic_int DestructedCount = 0;

    std::unordered_set<void *> AllocatedPointers;

    void *allocate(std::size_t sz)
    {
        ++AllocationCount;
        void *ptr = std::malloc(sz);
        AllocatedPointers.insert(ptr);
        return ptr;
    }

    void deallocate(void *ptr, std::size_t)
    {
        ++DeallocationCount;
        AllocatedPointers.erase(ptr);
        std::free(ptr);
    }
};

class TestAllocator
{
public:
    std::shared_ptr<SharedAllocatorState> State;

    TestAllocator(std::shared_ptr<SharedAllocatorState> state) : State(state)
    {
    }

    TestAllocator(const TestAllocator &other)
    {
        State = other.State;
        ++State->CopyConstructedCount;
    }

    TestAllocator &operator=(const TestAllocator &other)
    {
        if (this != &other)
        {
            State = other.State;
            ++State->CopyConstructedCount;
        }
        return *this;
    }

    TestAllocator(TestAllocator &&other) noexcept
    {
        State = std::move(other.State);
        ++State->MoveConstructedCount;
    }

    TestAllocator &operator=(TestAllocator &&other) noexcept
    {
        if (this != &other)
        {
            State = std::move(other.State);
            ++State->MoveConstructedCount;
        }
        return *this;
    }

    ~TestAllocator()
    {
        if (State)
        {
            ++State->DestructedCount;
        }
    }

    void *allocate(std::size_t sz)
    {
        return State->allocate(sz);
    }

    void deallocate(void *ptr, std::size_t sz)
    {
        State->deallocate(ptr, sz);
    }
};

template <typename AllocatorT>
struct TaskImplWithCustomAllocator :
    Cortado::Common::STLAtomic,
    Cortado::Common::STLExceptionHandler,
    Cortado::DefaultScheduler
{
    using Event = Cortado::DefaultEvent;
    using Allocator = AllocatorT;
};

TEST(CoroutineAllocatorTest, BasicAllocationDeallocationWithMovableAllocator)
{
    using Task =
        Cortado::Task<void, TaskImplWithCustomAllocator<TestAllocator>>;

    auto state = std::make_shared<SharedAllocatorState>();
    TestAllocator allocator{state};

    auto taskLambda = [](TestAllocator &) -> Task
    {
        co_return;
    };

    taskLambda(allocator).Get();

    EXPECT_EQ(1, state->AllocationCount)
        << "Expected 1 allocation for coroutine frame";

    EXPECT_EQ(1, state->DeallocationCount)
        << "Expected 1 deallocation for coroutine frame";

    EXPECT_EQ(1, state->CopyConstructedCount)
        << "Expected 1 copy constructor during `new` call";

    EXPECT_EQ(1, state->MoveConstructedCount)
        << "Expected 1 move constructor call during `delete`";

    EXPECT_EQ(1, state->DestructedCount)
        << "Only explicit destructor in `delete` is expected";

    EXPECT_TRUE(state->AllocatedPointers.empty())
        << "All allocated pointers must be deallocated";
}

TEST(CoroutineAllocatorTest, BasicAllocationDeallocationWithNonMovableAllocator)
{
    struct NonMovableAllocator : TestAllocator
    {
        using TestAllocator::TestAllocator;
        NonMovableAllocator(const NonMovableAllocator &other) = default;
        NonMovableAllocator &operator=(const NonMovableAllocator &other) =
            default;
        NonMovableAllocator(NonMovableAllocator &&) = delete;
        NonMovableAllocator &operator=(NonMovableAllocator &&) = delete;
    };

    using Task =
        Cortado::Task<void, TaskImplWithCustomAllocator<NonMovableAllocator>>;

    auto state = std::make_shared<SharedAllocatorState>();
    NonMovableAllocator allocator{state};

    auto taskLambda = [](NonMovableAllocator &) -> Task
    {
        co_return;
    };

    taskLambda(allocator).Get();

    EXPECT_EQ(1, state->AllocationCount)
        << "Expected 1 allocation for coroutine frame";

    EXPECT_EQ(1, state->DeallocationCount)
        << "Expected 1 deallocation for coroutine frame";

    EXPECT_EQ(2, state->CopyConstructedCount)
        << "Expected 1 copy constructor during `new` call";

    EXPECT_EQ(0, state->MoveConstructedCount) << "Moving must not be possible";

    EXPECT_EQ(2, state->DestructedCount)
        << "Only explicit destructor in `delete` is expected";

    EXPECT_TRUE(state->AllocatedPointers.empty())
        << "All allocated pointers must be deallocated";
}

TEST(CoroutineAllocatorTest, AllocationFailureBehaivor)
{
    struct ThrowingAllocator
    {
        void *allocate(std::size_t)
        {
            throw std::bad_alloc{};
        }
        void deallocate(void *, std::size_t)
        {
        }
    };

    using Task =
        Cortado::Task<void, TaskImplWithCustomAllocator<ThrowingAllocator>>;

    auto taskLambda = []() -> Task
    {
        co_return;
    };

    EXPECT_THROW(taskLambda(), std::bad_alloc);
}
