#ifndef FLORID_DETAIL_LATEST_MAILBOX_HPP
#define FLORID_DETAIL_LATEST_MAILBOX_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>

namespace florid::detail {

// A fixed-size SPSC mailbox. Publishing replaces an unread value; the consumer
// observes at most one frame and always eventually receives the latest one.
// publish() must always be called by the same thread, as must try_take().
template <std::size_t FrameCapacity>
class LatestMailbox {
public:
  struct Frame {
    std::array<std::uint8_t, FrameCapacity> m_data{};
    std::size_t m_size{0};
  };

  LatestMailbox() : m_middle(&m_slots[1]), m_front(&m_slots[0]), m_back(&m_slots[2]) {}

  LatestMailbox(const LatestMailbox &) = delete;
  LatestMailbox &operator=(const LatestMailbox &) = delete;

  bool publish(std::span<const std::uint8_t> s_data) noexcept {
    if (s_data.size() > FrameCapacity) return false;

    std::copy(s_data.begin(), s_data.end(), m_back->m_data.begin());
    m_back->m_size = s_data.size();

    // Ownership of the three slots is exchanged atomically. The producer only
    // writes m_back, while the consumer only reads m_front.
    m_back = m_middle.exchange(m_back, std::memory_order_acq_rel);
    m_dirty.store(true, std::memory_order_release);
    return true;
  }

  bool try_take(Frame &s_frame) noexcept {
    if (!m_dirty.exchange(false, std::memory_order_acq_rel)) return false;

    m_front = m_middle.exchange(m_front, std::memory_order_acq_rel);
    s_frame = *m_front;
    return true;
  }

  bool has_pending() const noexcept {
    return m_dirty.load(std::memory_order_acquire);
  }

private:
  std::array<Frame, 3> m_slots{};
  std::atomic<Frame *> m_middle;
  Frame *m_front;
  Frame *m_back;
  std::atomic<bool> m_dirty{false};
};

} // namespace florid::detail

#endif // FLORID_DETAIL_LATEST_MAILBOX_HPP
