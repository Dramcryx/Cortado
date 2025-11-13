/// @file EventBase.h
/// Shared code between all platform-specific futex-based events.
///

#ifndef CORTADO_COMMON_EVENT_BASE_H
#define CORTADO_COMMON_EVENT_BASE_H

// STL
//
#include <atomic>
#include <cstdint>

namespace Cortado::Common
{

template <typename EventImplT>
class EventBase
{
public:
    /// @brief Constructor.
    ///
    EventBase() = default;

    /// @brief Non-copyable.
    ///
    EventBase(const EventBase &) = delete;

    /// @brief Non-copyable.
    ///
    EventBase &operator=(const EventBase &) = delete;

    /// @brief Non-movable.
    ///
    EventBase(EventBase &&) = delete;

    /// @brief Non-movable.
    ///
    EventBase &operator=(EventBase &&) = delete;

    /// @brief Destructor.
    ///
    ~EventBase() = default;

    /// @brief Concept contract: Test if event is set already.
    /// @return true if event is set, false otherwise.
    ///
    bool IsSet() const noexcept
    {
        return m_state.load(std::memory_order_acquire) != 0;
    }

    /// @brief Concept contact: Singal the event.
    ///
    void Set() noexcept
    {
        m_state.store(1, std::memory_order_release);
        static_cast<EventImplT *>(this)->WakeAll(&m_state);
    }

    /// @brief Concept contract: Wait for event to be set.
    ///
    void Wait() noexcept
    {
        while (!IsSet())
        {
            static_cast<EventImplT *>(this)->WaitForImpl(&m_state,
                                                         UINT64_MAX);
        }
    }

    /// @brief Concept contract: Wait for event to be set in a period of time.
    /// @param timeToWaitMs How many milliseconds to wait.
    /// @returns true if event was set in timeToWaitMs, false otherwise.
    ///
    bool WaitFor(unsigned timeout_ms) noexcept
    {
        const uint64_t deadlineNs =
            static_cast<uint64_t>(timeout_ms) * 1'000'000ULL;

        for (;;)
        {
            if (IsSet())
            {
                return true;
            }

            if (!static_cast<EventImplT *>(this)->WaitForImpl(&m_state,
                                                              deadlineNs))
            {
                return false; // timed out
            }
        }
    }

private:
    std::atomic<int> m_state{0};
};

} // namespace Cortado::Common

#endif // CORTADO_COMMON_EVENT_BASE_H
