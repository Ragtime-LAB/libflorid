#ifndef FLORID_DETAIL_RECEIVE_CALLBACK_GATE_HPP
#define FLORID_DETAIL_RECEIVE_CALLBACK_GATE_HPP

#include "florid/detail/Transport.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace florid::detail {

// Publishes a receive callback without putting a mutex on the receive hot
// path. Clearing or replacing the callback closes the gate first and waits for
// every invocation that already entered it to return.
class ReceiveCallbackGate {
public:
    using Callback = Transport::ReceiveFunctor;

    ReceiveCallbackGate() = default;

    ReceiveCallbackGate(const ReceiveCallbackGate&) = delete;
    ReceiveCallbackGate& operator=(const ReceiveCallbackGate&) = delete;

    void set(Callback s_callback, void* s_context) {
        std::lock_guard<std::mutex> s_lock(m_update_mutex);
        s_closeAndWait();

        m_callback = s_callback;
        m_context = s_callback != nullptr ? s_context : nullptr;
        if (s_callback != nullptr) {
            m_state.store(0, std::memory_order_release);
        }
    }

    void clear() { set(nullptr, nullptr); }

    bool invoke(const std::uint8_t* s_data, std::size_t s_size) {
        if (!s_tryEnter()) return false;

        struct ExitGuard {
            ReceiveCallbackGate& m_gate;
            ~ExitGuard() { m_gate.s_leave(); }
        } s_exit{*this};

        m_callback(m_context, s_data, s_size);
        return true;
    }

private:
    static constexpr std::uint32_t s_kClosed = UINT32_C(1) << 31;
    static constexpr std::uint32_t s_kCountMask = ~s_kClosed;

    bool s_tryEnter() noexcept {
        auto s_state = m_state.load(std::memory_order_acquire);
        for (;;) {
            if ((s_state & s_kClosed) != 0 ||
                (s_state & s_kCountMask) == s_kCountMask) {
                return false;
            }
            if (m_state.compare_exchange_weak(
                    s_state, s_state + 1, std::memory_order_acquire,
                    std::memory_order_relaxed)) {
                return true;
            }
        }
    }

    void s_leave() noexcept {
        const auto s_previous =
            m_state.fetch_sub(1, std::memory_order_acq_rel);
        if ((s_previous & s_kClosed) != 0 &&
            (s_previous & s_kCountMask) == 1) {
            m_state.notify_all();
        }
    }

    void s_closeAndWait() noexcept {
        auto s_state =
            m_state.fetch_or(s_kClosed, std::memory_order_acq_rel) | s_kClosed;
        while ((s_state & s_kCountMask) != 0) {
            m_state.wait(s_state, std::memory_order_acquire);
            s_state = m_state.load(std::memory_order_acquire);
        }
    }

    std::mutex m_update_mutex;
    std::atomic<std::uint32_t> m_state{s_kClosed};
    Callback m_callback{};
    void* m_context{};
};

} // namespace florid::detail

#endif // FLORID_DETAIL_RECEIVE_CALLBACK_GATE_HPP
