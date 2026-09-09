#pragma once

#include "florid/Exceptions.hpp"
#include <atomic>
#include <cstdint>
#include <utility>

namespace florid::detail {
// Lifecycle callers serialize invalidate(). Sends only use atomics; invalidate
// waits for submissions that already entered, before a mode RPC/new session.
class ControlGeneration {
public:
    ControlGeneration() {
        if (!m_generation.is_lock_free() || !m_inflight.is_lock_free() || !m_draining.is_lock_free())
            throw ControlException("Control sessions require lock-free atomics on this platform");
    }
    std::uint64_t current() const noexcept { return m_generation.load(); }
    void invalidate() noexcept {
        m_draining.store(true);
        m_generation.fetch_add(1);
        for (auto s_count = m_inflight.load(); s_count != 0; s_count = m_inflight.load())
            m_inflight.wait(s_count);
        m_draining.store(false);
    }
    template <typename F>
    void send(std::uint64_t s_generation, F&& s_send) {
        if (s_generation != current()) throw ControlException("Control handle has expired");
        m_inflight.fetch_add(1);
        struct Leave {
            ControlGeneration& m_owner;
            ~Leave() {
                if (m_owner.m_inflight.fetch_sub(1) == 1 && m_owner.m_draining.load())
                    m_owner.m_inflight.notify_all();
            }
        } s_leave{*this};
        // SC ordering across the two atomics closes the check/enter race with
        // invalidate(): either it sees our entry, or we see its new generation.
        if (s_generation != current()) throw ControlException("Control handle has expired");
        std::forward<F>(s_send)();
    }
private:
    std::atomic<std::uint64_t> m_generation{1};
    std::atomic<unsigned> m_inflight{0};
    std::atomic<bool> m_draining{false};
};
} // namespace florid::detail
