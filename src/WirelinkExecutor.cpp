#include "florid/detail/WirelinkExecutor.hpp"

#include <chrono>
#include <cstring>
#include <system_error>

namespace florid::detail {

namespace {
constexpr std::size_t s_kPollBudget = 64;

bool s_isExpectedServiceResult(int s_result) {
    return s_result == WL_OK || s_result == WL_ERR_NO_DATA ||
           s_result == WL_ERR_WOULD_BLOCK;
}
} // namespace

WirelinkExecutor::~WirelinkExecutor() {
    stop();
}

int WirelinkExecutor::initialize(const wl_config_t& s_config,
                                 const wl_storage_t& s_storage) {
    if (state() != State::kUninitialized) return WL_ERR_INVALID_STATE;
    if (s_config.max_payload_len > s_kMaximumCommandPayload) {
        return WL_ERR_INVALID_ARG;
    }

    const int s_result = wl_init(&m_context, &s_config, &s_storage);
    if (s_result != WL_OK) return s_result;
    m_state.store(State::kReady, std::memory_order_release);
    return WL_OK;
}

int WirelinkExecutor::setHooks(const WirelinkExecutorHooks& s_hooks) {
    if (state() != State::kReady) return WL_ERR_INVALID_STATE;
    m_hooks = s_hooks;
    return WL_OK;
}

int WirelinkExecutor::setSink(wl_sink_fn s_sink, void* s_user_data) {
    if (state() != State::kReady) return WL_ERR_INVALID_STATE;
    return wl_set_sink(&m_context, s_sink, s_user_data);
}

int WirelinkExecutor::start() {
    State s_expected = State::kReady;
    if (!m_state.compare_exchange_strong(s_expected, State::kRunning,
                                         std::memory_order_acq_rel)) {
        return WL_ERR_INVALID_STATE;
    }

    m_stop_requested.store(false, std::memory_order_release);
    m_accepting.store(true, std::memory_order_release);
    try {
        m_thread = std::thread([this] { s_run(); });
    } catch (const std::system_error&) {
        m_accepting.store(false, std::memory_order_release);
        m_state.store(State::kReady, std::memory_order_release);
        return WL_ERR_IO;
    }
    notify();
    return WL_OK;
}

void WirelinkExecutor::requestStop() noexcept {
    m_accepting.store(false, std::memory_order_release);
    m_stop_requested.store(true, std::memory_order_release);

    State s_expected = State::kRunning;
    (void)m_state.compare_exchange_strong(s_expected, State::kStopping,
                                          std::memory_order_acq_rel);
    notify();
}

void WirelinkExecutor::stop() noexcept {
    const State s_current = state();
    if (s_current == State::kUninitialized) return;
    if (s_current == State::kStopped) {
        if (m_thread.joinable() &&
            m_thread.get_id() != std::this_thread::get_id()) {
            m_thread.join();
        }
        return;
    }

    if (s_current == State::kReady) {
        m_accepting.store(false, std::memory_order_release);
        if (m_hooks.m_quiesce != nullptr) {
            m_hooks.m_quiesce(m_hooks.m_user_data);
        }
        (void)wl_set_sink(&m_context, nullptr, nullptr);
        m_state.store(State::kStopped, std::memory_order_release);
        return;
    }

    requestStop();
    if (m_thread.joinable() && m_thread.get_id() != std::this_thread::get_id()) {
        m_thread.join();
    }
}

int WirelinkExecutor::feedBytes(const std::uint8_t* s_data,
                                std::size_t s_size,
                                std::size_t& s_accepted) noexcept {
    s_accepted = 0;
    if (!m_accepting.load(std::memory_order_acquire)) {
        return WL_ERR_CANCELLED;
    }

    m_producers_in_flight.fetch_add(1, std::memory_order_acq_rel);
    if (!m_accepting.load(std::memory_order_acquire)) {
        if (m_producers_in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            m_producers_in_flight.notify_all();
        }
        return WL_ERR_CANCELLED;
    }

    m_stats.m_feed_calls.fetch_add(1, std::memory_order_relaxed);
    const int s_result = wl_feed_bytes(&m_context, s_data, s_size, &s_accepted);
    m_stats.m_feed_bytes.fetch_add(s_accepted, std::memory_order_relaxed);
    if (s_result == WL_ERR_WOULD_BLOCK || s_result == WL_ERR_NO_SPACE) {
        m_stats.m_feed_backpressure.fetch_add(1, std::memory_order_relaxed);
    }

    if (m_producers_in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        m_producers_in_flight.notify_all();
    }
    notify();
    return s_result;
}

void WirelinkExecutor::notify() noexcept {
    m_wake_generation.fetch_add(1, std::memory_order_release);
    m_wake.release();
}

int WirelinkExecutor::submitLatest(std::uint16_t s_message_id,
                                   const std::uint8_t* s_payload,
                                   std::size_t s_payload_size) noexcept {
    if (s_message_id == 0 || s_payload_size > s_kMaximumCommandPayload ||
        (s_payload == nullptr && s_payload_size != 0)) {
        return WL_ERR_INVALID_ARG;
    }
    if (!m_accepting.load(std::memory_order_acquire)) {
        return WL_ERR_CANCELLED;
    }

    {
        std::lock_guard<std::mutex> s_lock(m_command_mutex);
        if (!m_accepting.load(std::memory_order_relaxed)) {
            return WL_ERR_CANCELLED;
        }
        LatestLane* s_lane = nullptr;
        for (auto& s_candidate : m_latest_lanes) {
            if (s_candidate.m_valid &&
                s_candidate.m_command.m_message_id == s_message_id) {
                s_lane = &s_candidate;
                m_stats.m_latest_coalesced.fetch_add(
                    1, std::memory_order_relaxed);
                break;
            }
        }
        if (s_lane == nullptr) {
            for (auto& s_candidate : m_latest_lanes) {
                if (!s_candidate.m_valid) {
                    s_lane = &s_candidate;
                    break;
                }
            }
        }
        if (s_lane == nullptr) {
            m_stats.m_latest_queue_full.fetch_add(1,
                                                  std::memory_order_relaxed);
            return WL_ERR_QUEUE_FULL;
        }

        s_lane->m_command.m_ticket = 0;
        s_lane->m_command.m_generation = m_next_generation++;
        if (m_next_generation == 0) m_next_generation = 1;
        s_lane->m_command.m_message_id = s_message_id;
        s_lane->m_command.m_payload_size =
            static_cast<std::uint16_t>(s_payload_size);
        if (s_payload_size != 0) {
            std::memcpy(s_lane->m_command.m_payload.data(), s_payload,
                        s_payload_size);
        }
        s_lane->m_valid = true;
    }

    m_stats.m_latest_submitted.fetch_add(1, std::memory_order_relaxed);
    notify();
    return WL_OK;
}

int WirelinkExecutor::submitReliable(std::uint16_t s_message_id,
                                     const std::uint8_t* s_payload,
                                     std::size_t s_payload_size,
                                     std::uint64_t& s_ticket) noexcept {
    s_ticket = 0;
    if (s_message_id == 0 || s_payload_size > s_kMaximumCommandPayload ||
        (s_payload == nullptr && s_payload_size != 0)) {
        return WL_ERR_INVALID_ARG;
    }
    if (!m_accepting.load(std::memory_order_acquire)) {
        return WL_ERR_CANCELLED;
    }

    {
        std::lock_guard<std::mutex> s_lock(m_command_mutex);
        if (!m_accepting.load(std::memory_order_relaxed)) {
            return WL_ERR_CANCELLED;
        }
        if (m_reliable_count == s_kReliableQueueCapacity) {
            m_stats.m_reliable_queue_full.fetch_add(1, std::memory_order_relaxed);
            return WL_ERR_QUEUE_FULL;
        }

        Command& s_command = m_reliable_queue[m_reliable_tail];
        s_command.m_ticket = m_next_ticket++;
        if (m_next_ticket == 0) m_next_ticket = 1;
        s_command.m_generation = 0;
        s_command.m_message_id = s_message_id;
        s_command.m_payload_size = static_cast<std::uint16_t>(s_payload_size);
        if (s_payload_size != 0) {
            std::memcpy(s_command.m_payload.data(), s_payload, s_payload_size);
        }
        s_ticket = s_command.m_ticket;
        m_reliable_tail = (m_reliable_tail + 1) % s_kReliableQueueCapacity;
        ++m_reliable_count;
    }

    m_stats.m_reliable_submitted.fetch_add(1, std::memory_order_relaxed);
    notify();
    return WL_OK;
}

wl_time_ms_t WirelinkExecutor::s_nowMs() noexcept {
    using namespace std::chrono;
    return static_cast<wl_time_ms_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void WirelinkExecutor::s_run() noexcept {
    while (!m_stop_requested.load(std::memory_order_acquire)) {
        for (std::size_t s_index = 0;
             s_index < s_kPollBudget && m_wake.try_acquire(); ++s_index) {
        }
        const std::uint64_t s_observed_wake =
            m_wake_generation.load(std::memory_order_acquire);
        const wl_time_ms_t s_now = s_nowMs();

        if (m_hooks.m_service != nullptr) {
            const int s_result = m_hooks.m_service(m_hooks.m_user_data);
            if (!s_isExpectedServiceResult(s_result)) {
                m_stats.m_service_errors.fetch_add(1, std::memory_order_relaxed);
            }
        }

        bool s_progress = s_pollEvents(s_now);
        if (m_stop_requested.load(std::memory_order_acquire)) break;

        if (s_dispatchOne()) {
            s_progress = true;
        }
        if (s_progress) continue;

        wl_poll_hint_t s_hint{};
        const int s_hint_result = wl_poll_get_hint(&m_context, s_nowMs(), &s_hint);
        if (s_hint_result != WL_OK) {
            m_stats.m_poll_errors.fetch_add(1, std::memory_order_relaxed);
        } else if (s_hint.work_pending != 0) {
            continue;
        }

        if (m_stop_requested.load(std::memory_order_acquire) ||
            m_wake_generation.load(std::memory_order_acquire) !=
                s_observed_wake) {
            continue;
        }
        if (s_hint_result == WL_OK &&
            s_hint.next_deadline_ms != WL_POLL_NO_DEADLINE_MS) {
            (void)m_wake.try_acquire_for(
                std::chrono::milliseconds(s_hint.next_deadline_ms));
        } else {
            m_wake.acquire();
        }
    }

    s_shutdownOnOwner();
    m_state.store(State::kStopped, std::memory_order_release);
}

bool WirelinkExecutor::s_pollEvents(wl_time_ms_t s_now_ms) noexcept {
    bool s_progress = false;
    for (std::size_t s_index = 0; s_index < s_kPollBudget; ++s_index) {
        wl_event_t s_event{};
        const int s_result = wl_poll(&m_context, s_now_ms, &s_event);
        if (s_result == WL_ERR_NO_DATA) break;
        if (s_result != WL_OK) {
            m_stats.m_poll_errors.fetch_add(1, std::memory_order_relaxed);
            s_progress = true;
            continue;
        }

        s_progress = true;
        s_handleEvent(s_event);
    }
    return s_progress;
}

bool WirelinkExecutor::s_dispatchOne() noexcept {
    if (m_active_reliable_valid) return false;

    Command s_reliable{};
    Command s_latest{};
    const bool s_has_reliable = s_peekReliable(s_reliable);
    const bool s_has_latest = s_peekLatest(s_latest);
    if (!s_has_reliable && !s_has_latest) return false;

    // Alternate when both classes are pending. LATEST gets the first free TX
    // gap, while a reliable FIFO entry is guaranteed the following gap.
    const bool s_send_latest =
        s_has_latest && (!s_has_reliable || !m_latest_had_last_turn);
    if (!s_send_latest) {
        wl_tx_handle_t s_handle{};
        const int s_result = wl_send_reliable(
            &m_context, s_reliable.m_message_id,
            s_reliable.m_payload.data(), s_reliable.m_payload_size, &s_handle);
        if (s_result == WL_OK) {
            s_popReliable(s_reliable.m_ticket);
            m_active_reliable = s_reliable;
            m_active_handle = s_handle;
            m_active_reliable_valid = true;
            m_latest_had_last_turn = false;
            m_stats.m_reliable_dispatched.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        if (s_result == WL_ERR_BUSY || s_result == WL_ERR_WOULD_BLOCK) {
            return false;
        }

        s_popReliable(s_reliable.m_ticket);
        m_latest_had_last_turn = false;
        m_stats.m_reliable_failed.fetch_add(1, std::memory_order_relaxed);
        s_complete(s_reliable, WL_TX_STATE_FAILED, s_result, 0);
        return true;
    }

    const int s_result = wl_send_unreliable(
        &m_context, s_latest.m_message_id, s_latest.m_payload.data(),
        s_latest.m_payload_size);
    if (s_result == WL_OK) {
        s_removeLatest(s_latest.m_generation);
        m_latest_had_last_turn = true;
        m_stats.m_latest_dispatched.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    if (s_result == WL_ERR_BUSY || s_result == WL_ERR_WOULD_BLOCK) {
        return false;
    }

    s_removeLatest(s_latest.m_generation);
    m_latest_had_last_turn = true;
    m_stats.m_latest_failed.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void WirelinkExecutor::s_handleEvent(const wl_event_t& s_event) noexcept {
    if (s_event.type == WL_EVT_UNRELIABLE_RX ||
        s_event.type == WL_EVT_RELIABLE_RX) {
        m_stats.m_rx_events.fetch_add(1, std::memory_order_relaxed);
        if (m_hooks.m_on_event != nullptr) {
            m_hooks.m_on_event(m_hooks.m_user_data, m_context, s_event);
        } else {
            wl_event_release(&m_context, &s_event);
        }
        return;
    }

    if (s_event.type != WL_EVT_TX_SUCCESS &&
        s_event.type != WL_EVT_TX_TIMEOUT &&
        s_event.type != WL_EVT_TX_FAILED) {
        return;
    }
    if (!m_active_reliable_valid || s_event.handle != m_active_handle) {
        return;
    }

    wl_tx_result_t s_result{};
    int s_take_result = wl_tx_take(&m_context, m_active_handle, &s_result);
    if (s_take_result != WL_OK) {
        s_result.state = WL_TX_STATE_FAILED;
        s_result.result = s_take_result;
        s_result.retries_used = 0;
    }

    const Command s_command = m_active_reliable;
    m_active_reliable_valid = false;
    m_active_handle = 0;
    if (s_result.state == WL_TX_STATE_SUCCESS) {
        m_stats.m_reliable_completed.fetch_add(1, std::memory_order_relaxed);
    } else {
        m_stats.m_reliable_failed.fetch_add(1, std::memory_order_relaxed);
    }
    s_complete(s_command, s_result.state, s_result.result,
               s_result.retries_used);
}

void WirelinkExecutor::s_complete(const Command& s_command,
                                  wl_tx_state_t s_state, int s_result,
                                  std::uint16_t s_retries) noexcept {
    if (m_hooks.m_on_completion == nullptr) return;
    const WirelinkCommandResult s_completion{
        .m_ticket = s_command.m_ticket,
        .m_message_id = s_command.m_message_id,
        .m_state = s_state,
        .m_result = s_result,
        .m_retries_used = s_retries,
    };
    m_hooks.m_on_completion(m_hooks.m_user_data, s_completion);
}

void WirelinkExecutor::s_shutdownOnOwner() noexcept {
    m_accepting.store(false, std::memory_order_release);
    if (m_hooks.m_quiesce != nullptr) {
        m_hooks.m_quiesce(m_hooks.m_user_data);
    }

    std::uint32_t s_producers =
        m_producers_in_flight.load(std::memory_order_acquire);
    while (s_producers != 0) {
        m_producers_in_flight.wait(s_producers, std::memory_order_acquire);
        s_producers = m_producers_in_flight.load(std::memory_order_acquire);
    }

    if (m_hooks.m_service != nullptr) {
        const int s_service_result = m_hooks.m_service(m_hooks.m_user_data);
        if (!s_isExpectedServiceResult(s_service_result)) {
            m_stats.m_service_errors.fetch_add(1, std::memory_order_relaxed);
        }
    }

    if (m_active_reliable_valid) {
        (void)wl_tx_cancel(&m_context, m_active_handle);
        wl_tx_result_t s_ignored{};
        (void)wl_tx_take(&m_context, m_active_handle, &s_ignored);
        const Command s_command = m_active_reliable;
        m_active_reliable_valid = false;
        m_active_handle = 0;
        m_stats.m_reliable_cancelled.fetch_add(1, std::memory_order_relaxed);
        s_complete(s_command, WL_TX_STATE_CANCELLED, WL_ERR_CANCELLED, 0);
    }

    while (true) {
        Command s_command{};
        if (!s_peekReliable(s_command)) break;
        s_popReliable(s_command.m_ticket);
        m_stats.m_reliable_cancelled.fetch_add(1, std::memory_order_relaxed);
        s_complete(s_command, WL_TX_STATE_CANCELLED, WL_ERR_CANCELLED, 0);
    }

    {
        std::lock_guard<std::mutex> s_lock(m_command_mutex);
        std::uint64_t s_cancelled{};
        for (auto& s_lane : m_latest_lanes) {
            if (s_lane.m_valid) {
                s_lane.m_valid = false;
                ++s_cancelled;
            }
        }
        if (s_cancelled != 0) {
            m_stats.m_latest_cancelled.fetch_add(s_cancelled,
                                                 std::memory_order_relaxed);
        }
    }
    (void)wl_set_sink(&m_context, nullptr, nullptr);
}

bool WirelinkExecutor::s_peekReliable(Command& s_command) noexcept {
    std::lock_guard<std::mutex> s_lock(m_command_mutex);
    if (m_reliable_count == 0) return false;
    s_command = m_reliable_queue[m_reliable_head];
    return true;
}

void WirelinkExecutor::s_popReliable(std::uint64_t s_ticket) noexcept {
    std::lock_guard<std::mutex> s_lock(m_command_mutex);
    if (m_reliable_count == 0 ||
        m_reliable_queue[m_reliable_head].m_ticket != s_ticket) {
        return;
    }
    m_reliable_head = (m_reliable_head + 1) % s_kReliableQueueCapacity;
    --m_reliable_count;
}

bool WirelinkExecutor::s_peekLatest(Command& s_command) noexcept {
    std::lock_guard<std::mutex> s_lock(m_command_mutex);
    for (std::size_t s_offset = 0; s_offset < m_latest_lanes.size();
         ++s_offset) {
        const std::size_t s_index =
            (m_latest_cursor + s_offset) % m_latest_lanes.size();
        if (!m_latest_lanes[s_index].m_valid) continue;
        s_command = m_latest_lanes[s_index].m_command;
        m_latest_cursor = (s_index + 1) % m_latest_lanes.size();
        return true;
    }
    return false;
}

void WirelinkExecutor::s_removeLatest(std::uint64_t s_generation) noexcept {
    std::lock_guard<std::mutex> s_lock(m_command_mutex);
    for (auto& s_lane : m_latest_lanes) {
        if (s_lane.m_valid &&
            s_lane.m_command.m_generation == s_generation) {
            s_lane.m_valid = false;
            return;
        }
    }
}

WirelinkExecutorStats WirelinkExecutor::stats() const noexcept {
    return WirelinkExecutorStats{
        .m_feed_calls = m_stats.m_feed_calls.load(std::memory_order_relaxed),
        .m_feed_bytes = m_stats.m_feed_bytes.load(std::memory_order_relaxed),
        .m_feed_backpressure =
            m_stats.m_feed_backpressure.load(std::memory_order_relaxed),
        .m_rx_events = m_stats.m_rx_events.load(std::memory_order_relaxed),
        .m_poll_errors = m_stats.m_poll_errors.load(std::memory_order_relaxed),
        .m_service_errors =
            m_stats.m_service_errors.load(std::memory_order_relaxed),
        .m_latest_submitted =
            m_stats.m_latest_submitted.load(std::memory_order_relaxed),
        .m_latest_coalesced =
            m_stats.m_latest_coalesced.load(std::memory_order_relaxed),
        .m_latest_queue_full =
            m_stats.m_latest_queue_full.load(std::memory_order_relaxed),
        .m_latest_dispatched =
            m_stats.m_latest_dispatched.load(std::memory_order_relaxed),
        .m_latest_failed =
            m_stats.m_latest_failed.load(std::memory_order_relaxed),
        .m_latest_cancelled =
            m_stats.m_latest_cancelled.load(std::memory_order_relaxed),
        .m_reliable_submitted =
            m_stats.m_reliable_submitted.load(std::memory_order_relaxed),
        .m_reliable_queue_full =
            m_stats.m_reliable_queue_full.load(std::memory_order_relaxed),
        .m_reliable_dispatched =
            m_stats.m_reliable_dispatched.load(std::memory_order_relaxed),
        .m_reliable_completed =
            m_stats.m_reliable_completed.load(std::memory_order_relaxed),
        .m_reliable_failed =
            m_stats.m_reliable_failed.load(std::memory_order_relaxed),
        .m_reliable_cancelled =
            m_stats.m_reliable_cancelled.load(std::memory_order_relaxed),
    };
}

} // namespace florid::detail
