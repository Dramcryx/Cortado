#include <gtest/gtest.h>

// Cortado
//
#include <Cortado/Await.h>
#include <Cortado/Common/STLAtomic.h>

// STL
//
#include <mutex>
#include <unordered_map>

namespace
{
inline const char *GetCurrentTestName()
{
    return ::testing::UnitTest::GetInstance()->current_test_info()->name();
}
} // namespace

struct UserStorage
{
    inline static std::mutex TestResultMutex;
    inline static std::unordered_map<std::string,
                                     std::pair<unsigned long, unsigned long>>
        TestResultStorage;

    UserStorage() = default;

    ~UserStorage()
    {
        std::unique_lock lk{TestResultMutex};
        TestResultStorage[m_testName] = {BeforeSuspendCallCount.load(),
                                         BeforeResumeCallCount.load()};
    }

    Cortado::Common::STLAtomic::Atomic BeforeSuspendCallCount{0};
    Cortado::Common::STLAtomic::Atomic BeforeResumeCallCount{0};

private:
    std::string m_testName = GetCurrentTestName();
};

struct PreAndPostActions
{
    using AdditionalStorage = UserStorage;

    static void OnBeforeSuspend(UserStorage &s)
    {
        ++s.BeforeSuspendCallCount;
    }

    static void OnBeforeResume(UserStorage &s)
    {
        ++s.BeforeResumeCallCount;
    }
};

struct DefaultTaskImplWithAdditionalStorage :
    Cortado::DefaultTaskImpl,
    PreAndPostActions
{
};

template <typename T>
using Task2 = Cortado::Task<T, DefaultTaskImplWithAdditionalStorage>;

TEST(DefaultTaskWithAdditionalStorageTests, BasicTest)
{
    auto task = []() -> Task2<int>
    {
        co_return 1;
    };

    task().Get();

    std::unique_lock lk{UserStorage::TestResultMutex};
    auto [beforeSuspend, _] =
        UserStorage::TestResultStorage[GetCurrentTestName()];
    EXPECT_EQ(1, beforeSuspend) << "Expected only one call in final awaiter";
}

TEST(DefaultTaskWithAdditionalStorageTests, ResumeBackgroundTest)
{
    static auto task = []() -> Task2<void>
    {
        co_await Cortado::ResumeBackground();
    };

    task().Get();

    unsigned long beforeSuspend = 0;
    unsigned long beforeResume = 0;

    std::string testName = GetCurrentTestName();

    for (;;)
    {
        std::unique_lock lk{UserStorage::TestResultMutex};
        if (!UserStorage::TestResultStorage.contains(testName))
        {
            continue;
        }

        std::tie(beforeSuspend, beforeResume) =
            UserStorage::TestResultStorage[testName];
        break;
    }
    EXPECT_EQ(2, beforeSuspend) << "Expected two calls";

    EXPECT_EQ(1, beforeResume)
        << "Expected one call in resume background awaiter";
}
