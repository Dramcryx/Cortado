/// @file MutexBase.h
/// Shared code between all platform-specific futexes
///

#ifndef CORTADO_COMMON_MUTEX_BASE_H
#define CORTADO_COMMON_MUTEX_BASE_H

// STL
//
#include <atomic>

namespace Cortado::Common
{

template <typename MutexImplT>
class MutexBase
{
public:
    /// @brief Constructor.
    ///
    MutexBase() = default;

    /// @brief Non-copyable.
    ///
    MutexBase(const MutexBase &) = delete;

    /// @brief Non-copyable.
    ///
    MutexBase &operator=(const MutexBase &) = delete;

    /// @brief Non-movable.
    ///
    MutexBase(MutexBase &&) = delete;

    /// @brief Non-movable.
    ///
    MutexBase &operator=(MutexBase &&) = delete;

    /// @brief Destructor.
    ///
    ~MutexBase() = default;

    /// @brief Concept contract: Lock mutex, waiting if needed.
    ///
    void lock() noexcept
    {
        int expected = 0;

        // Fast path
        //
        if (m_state.compare_exchange_strong(expected,
                                            1,
                                            std::memory_order_acquire))
        {
            return;
        }

        // Slow path
        for (;;)
        {
            expected = 0;

            if (m_state.compare_exchange_strong(expected,
                                                1,
                                                std::memory_order_acquire))
            {
                return;
            }

            static_cast<MutexImplT *>(this)->WaitOnAddress(&m_state);
        }
    }

    /// @brief Concept contract: Try locking mutex without waiting.
    ///
    bool try_lock() noexcept
    {
        int expected = 0;
        return m_state.compare_exchange_strong(expected,
                                               1,
                                               std::memory_order_acquire);
    }

    /// @brief Concept contract: Unlock mutex and wake one waiter if any.
    ///
    void unlock() noexcept
    {
        m_state.store(0, std::memory_order_release);
        static_cast<MutexImplT *>(this)->WakeOne(&m_state);
    }

private:
    std::atomic<int> m_state{0};
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_MUTEX_BASE_H
