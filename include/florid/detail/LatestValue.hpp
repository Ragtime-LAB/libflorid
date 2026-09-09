#pragma once

#include "florid/Exceptions.hpp"
#include <wirelink/latest.h>
#include <array>
#include <cstring>
#include <type_traits>

namespace florid::detail {

// One logical publisher and one logical reader, with three permanent slots.
// Caller-side serialization may combine multiple callers into one reader/writer;
// neither side may share that serialization lock with the opposite side.
template <typename T>
class LatestValue {
    static_assert(std::is_trivially_copyable_v<T>);
public:
    LatestValue() {
        const wl_latest_config_t s_config{sizeof(T), alignof(T), 0};
        const wl_latest_storage_t s_storage{m_values.data(), sizeof(m_values)};
        if (wl_latest_init(&m_mailbox, &s_config, &s_storage) != WL_OK)
            throw ControlException("Cannot initialize a lock-free latest-value mailbox");
    }
    LatestValue(const LatestValue&) = delete;
    LatestValue& operator=(const LatestValue&) = delete;
    LatestValue(LatestValue&&) = delete;
    LatestValue& operator=(LatestValue&&) = delete;

    int publish(const T& s_value) noexcept {
        wl_latest_write_claim_t s_claim{};
        const int s_result = wl_latest_write_claim(&m_mailbox, &s_claim);
        if (s_result != WL_OK) return s_result;
        std::memcpy(s_claim.value, &s_value, sizeof(T));
        return wl_latest_write_publish(&m_mailbox, &s_claim);
    }

    // Copy only an unseen publication. NO_DATA leaves the caller's cache intact.
    int read(T& s_value) noexcept {
        wl_latest_view_t s_view{};
        const int s_result = wl_latest_read_acquire(&m_mailbox, &s_view);
        if (s_result != WL_OK) return s_result;
        std::memcpy(&s_value, s_view.value, sizeof(T));
        return wl_latest_read_release(&m_mailbox, &s_view);
    }

private:
    wl_latest_t m_mailbox{};
    std::array<T, WL_LATEST_SLOT_COUNT> m_values{};
};
} // namespace florid::detail
