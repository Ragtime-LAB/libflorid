#include "../protocol/tests/support/managed_rpc_wire.hpp"
#include <florid/UnderwaterTeleop.hpp>
#include <florid/detail/Transport.hpp>
#include <wirelink/bulk.h>
#include <wirelink/crc.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace florid::detail {
struct UnderwaterTeleopTestAccess {
    static UnderwaterConnectionResult connect(std::unique_ptr<Transport> s_transport) {
        return UnderwaterTeleop::s_connectTransport(std::move(s_transport), std::chrono::seconds(1));
    }
};
}
namespace {
using namespace florid;
using namespace std::chrono_literals;
using GetRequest = test_rpc::Envelope<get_underwater_status_request_t, true,
    get_underwater_status_request_encode, get_underwater_status_request_decode>;
using GetResponse = test_rpc::Envelope<get_underwater_status_response_t, false,
    get_underwater_status_response_encode, get_underwater_status_response_decode>;
using SetRequest = test_rpc::Envelope<set_underwater_mode_request_t, true,
    set_underwater_mode_request_encode, set_underwater_mode_request_decode>;
using SetResponse = test_rpc::Envelope<set_underwater_mode_response_t, false,
    set_underwater_mode_response_encode, set_underwater_mode_response_decode>;
void require(bool s_ok, const char* s_reason) {
    if (!s_ok) throw std::runtime_error(s_reason);
}
using GetUnderwaterSettingsRequest = test_rpc::Envelope<get_underwater_settings_request_t, true,
    get_underwater_settings_request_encode, get_underwater_settings_request_decode>;
using GetUnderwaterSettingsResponse = test_rpc::Envelope<get_underwater_settings_response_t, false,
    get_underwater_settings_response_encode, get_underwater_settings_response_decode>;
using SetUnderwaterSettingsRequest = test_rpc::Envelope<set_underwater_settings_request_t, true,
    set_underwater_settings_request_encode, set_underwater_settings_request_decode>;
using SetUnderwaterSettingsResponse = test_rpc::Envelope<set_underwater_settings_response_t, false,
    set_underwater_settings_response_encode, set_underwater_settings_response_decode>;
using GetUnderwaterDefaultSettingsRequest = test_rpc::Envelope<get_underwater_default_settings_request_t, true,
    get_underwater_default_settings_request_encode, get_underwater_default_settings_request_decode>;
using GetUnderwaterDefaultSettingsResponse = test_rpc::Envelope<get_underwater_default_settings_response_t, false,
    get_underwater_default_settings_response_encode, get_underwater_default_settings_response_decode>;
underwater_settings_t settingsFixture() {
    underwater_settings_t s_settings{};
    float s_next = -2.75f;
    s_settings.has_angle_scale = true;
    for (auto& s_value : s_settings.angle_scale) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_angle_offset_rad = true;
    for (auto& s_value : s_settings.angle_offset_rad) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_kp = true;
    for (auto& s_value : s_settings.kp) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_kd = true;
    for (auto& s_value : s_settings.kd) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_gravity_scale = true;
    for (auto& s_value : s_settings.gravity_scale) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_friction_coulomb_positive_nm = true;
    for (auto& s_value : s_settings.friction_coulomb_positive_nm) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_friction_coulomb_negative_nm = true;
    for (auto& s_value : s_settings.friction_coulomb_negative_nm) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_friction_viscous_nm_per_rad_s = true;
    for (auto& s_value : s_settings.friction_viscous_nm_per_rad_s) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_friction_deadband_rad_s = true;
    for (auto& s_value : s_settings.friction_deadband_rad_s) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_friction_transition_rad_s = true;
    for (auto& s_value : s_settings.friction_transition_rad_s) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_friction_filter_time_constant_s = true;
    for (auto& s_value : s_settings.friction_filter_time_constant_s) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_force_gain = true;
    for (auto& s_value : s_settings.force_gain) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_force_low_gain = true;
    for (auto& s_value : s_settings.force_low_gain) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_force_velocity_filter_s = true;
    for (auto& s_value : s_settings.force_velocity_filter_s) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_force_load_filter_s = true;
    for (auto& s_value : s_settings.force_load_filter_s) { s_value = s_next; s_next += 0.125f; }
    s_settings.has_force_low_load_nm = s_settings.has_force_high_load_nm = true;
    s_settings.has_buzzer_alarm_enabled = s_settings.buzzer_alarm_enabled = true;
    s_settings.force_low_load_nm = 2.5f; s_settings.force_high_load_nm = 6.25f;
    return s_settings;
}
void checkSettings(const UnderwaterSettings& s_settings, const underwater_settings_t& s_wire) {
    require(std::equal(s_settings.m_angle_scale.begin(), s_settings.m_angle_scale.end(), s_wire.angle_scale), "angle_scale round trip");
    require(std::equal(s_settings.m_angle_offset_rad.begin(), s_settings.m_angle_offset_rad.end(), s_wire.angle_offset_rad), "angle_offset_rad round trip");
    require(std::equal(s_settings.m_kp.begin(), s_settings.m_kp.end(), s_wire.kp), "kp round trip");
    require(std::equal(s_settings.m_kd.begin(), s_settings.m_kd.end(), s_wire.kd), "kd round trip");
    require(std::equal(s_settings.m_gravity_scale.begin(), s_settings.m_gravity_scale.end(), s_wire.gravity_scale), "gravity_scale round trip");
    require(std::equal(s_settings.m_friction_coulomb_positive_nm.begin(), s_settings.m_friction_coulomb_positive_nm.end(), s_wire.friction_coulomb_positive_nm), "friction_coulomb_positive_nm round trip");
    require(std::equal(s_settings.m_friction_coulomb_negative_nm.begin(), s_settings.m_friction_coulomb_negative_nm.end(), s_wire.friction_coulomb_negative_nm), "friction_coulomb_negative_nm round trip");
    require(std::equal(s_settings.m_friction_viscous_nm_per_rad_s.begin(), s_settings.m_friction_viscous_nm_per_rad_s.end(), s_wire.friction_viscous_nm_per_rad_s), "friction_viscous_nm_per_rad_s round trip");
    require(std::equal(s_settings.m_friction_deadband_rad_s.begin(), s_settings.m_friction_deadband_rad_s.end(), s_wire.friction_deadband_rad_s), "friction_deadband_rad_s round trip");
    require(std::equal(s_settings.m_friction_transition_rad_s.begin(), s_settings.m_friction_transition_rad_s.end(), s_wire.friction_transition_rad_s), "friction_transition_rad_s round trip");
    require(std::equal(s_settings.m_friction_filter_time_constant_s.begin(), s_settings.m_friction_filter_time_constant_s.end(), s_wire.friction_filter_time_constant_s), "friction_filter_time_constant_s round trip");
    require(std::equal(s_settings.m_force_gain.begin(), s_settings.m_force_gain.end(), s_wire.force_gain), "force_gain round trip");
    require(std::equal(s_settings.m_force_low_gain.begin(), s_settings.m_force_low_gain.end(), s_wire.force_low_gain), "force_low_gain round trip");
    require(std::equal(s_settings.m_force_velocity_filter_s.begin(), s_settings.m_force_velocity_filter_s.end(), s_wire.force_velocity_filter_s), "force_velocity_filter_s round trip");
    require(std::equal(s_settings.m_force_load_filter_s.begin(), s_settings.m_force_load_filter_s.end(), s_wire.force_load_filter_s), "force_load_filter_s round trip");
    require(s_settings.m_buzzer_alarm_enabled == (!s_wire.has_buzzer_alarm_enabled || s_wire.buzzer_alarm_enabled),
            "buzzer alarm preference round trip");
    require(s_settings.m_force_low_load_nm == s_wire.force_low_load_nm &&
            s_settings.m_force_high_load_nm == s_wire.force_high_load_nm, "load threshold round trip");
}
struct State {
    std::atomic<unsigned> m_zeroing_count{};
    std::mutex m_stream_mutex;
    underwater_arm_command_t m_command{};
    std::uint32_t m_command_count{}, m_stream_sequence{};
    std::uint64_t m_control_session{1000};
    std::atomic<bool> m_streaming{};
    std::atomic<int> m_settings_count{}, m_settings_result{}, m_settings_source{UNDERWATER_SETTINGS_SOURCE_DEFAULTS};
    std::atomic<int> m_storage_error{};
    std::atomic<bool> m_disabled_masks{};
    std::uint32_t m_revision{7};
    underwater_settings_t m_settings{settingsFixture()};
    std::atomic<int> m_set_count{}, m_get_count{}, m_other_count{}, m_start_count{}, m_reboot_count{};
    std::atomic<int> m_mode{UNDERWATER_MODE_DAMP}, m_business_status{}, m_rejection{};
    std::atomic<bool> m_silent{}, m_hold_chunk{}, m_upgrading{}, m_fail_start{};
    std::atomic<std::size_t> m_written{};
    std::array<std::uint8_t, 8192> m_image{};
};

// Both peer core and its sink are driven by the host's one executor thread.
// This exercises real fragmented COBS, managed RPC envelopes, bulk receiver,
// SDK result translation and owner shutdown without physical USB hardware.
class PeerTransport final : public Transport {
public:
    explicit PeerTransport(std::shared_ptr<State> s_state) : m_state(std::move(s_state)) {}
    bool send(const std::uint8_t*, std::size_t) override { return false; }
    void setReceiveCallback(ReceiveFunctor, void*) override {}
    bool usesDirectWirelink() const noexcept override { return true; }
    TransportConnectionState connectionState() const noexcept override { return TransportConnectionState::kConnected; }
    int attachWirelink(wl_ctx_t& s_host, WakeFunctor, void*) noexcept override {
        m_host = &s_host;
        wl_config_t s_config{};
        s_config.max_payload_len = m_payload.size(); s_config.max_transmission_unit = m_tx.size();
        s_config.envelope = WL_ENVELOPE_COBS_STREAM; s_config.integrity = WL_INTEGRITY_NONE;
        s_config.session_id = 98765; s_config.max_retries = 2; s_config.ack_timeout_ms = 20;
        wl_storage_t s_storage{m_payload.data(), m_payload.size(), m_tx.data(), m_tx.size(),
            m_control.data(), m_control.size(), m_rx.data(), m_rx.size(), m_fallback.data(), m_fallback.size()};
        if (wl_init(&m_peer, &s_config, &s_storage) != WL_OK) return WL_ERR_INVALID_ARG;
        (void)wl_set_sink(&m_peer, [](void* s_context, wl_io_token_t, const std::uint8_t* s_data,
                                     std::size_t s_size) -> wl_sink_result_t {
            auto& s_self = *static_cast<PeerTransport*>(s_context);
            for (std::size_t s_i = 0; s_i < s_size;) {
                const auto s_chunk = std::min<std::size_t>(7, s_size - s_i);
                std::size_t s_accepted{};
                if (wl_feed_bytes(s_self.m_host, s_data + s_i, s_chunk, &s_accepted) != WL_OK ||
                    s_accepted != s_chunk) return WL_SINK_FAILED;
                s_i += s_chunk;
            }
            return WL_SINK_SENT;
        }, this);
        wl_bulk_receiver_config_t s_receiver{};
        s_receiver.max_object_length = m_state->m_image.size(); s_receiver.max_chunk_size = 128;
        s_receiver.write_alignment = 1; s_receiver.idle_timeout_ms = 2000;
        s_receiver.sink = {this,
            [](void* s_context, const wl_bulk_descriptor_t*, std::uint64_t* s_offset) -> wl_bulk_sink_result_t {
                auto& s_self = *static_cast<PeerTransport*>(s_context);
                s_self.m_state->m_written = 0; *s_offset = 0; return WL_BULK_SINK_OK;
            },
            [](void* s_context, std::uint32_t, std::uint64_t s_offset, const std::uint8_t* s_data,
               std::size_t s_size) -> wl_bulk_sink_result_t {
                auto& s_state = *static_cast<PeerTransport*>(s_context)->m_state;
                if (s_state.m_hold_chunk) return WL_BULK_SINK_BUSY;
                if (s_offset != s_state.m_written || s_size > s_state.m_image.size() - s_offset)
                    return WL_BULK_SINK_INVALID;
                std::copy_n(s_data, s_size, s_state.m_image.data() + s_offset);
                s_state.m_written += s_size;
                return WL_BULK_SINK_OK;
            },
            [](void* s_context, const wl_bulk_descriptor_t* s_descriptor) -> wl_bulk_sink_result_t {
                auto& s_state = *static_cast<PeerTransport*>(s_context)->m_state;
                if (wl_crc32c(s_state.m_image.data(), s_state.m_written) != s_descriptor->object_crc32c)
                    return WL_BULK_SINK_INTEGRITY_FAILED;
                s_state.m_upgrading = false;
                return WL_BULK_SINK_OK;
            },
            [](void* s_context, std::uint32_t, std::int32_t) {
                static_cast<PeerTransport*>(s_context)->m_state->m_upgrading = false;
            }};
        if (wl_bulk_receiver_init(&m_receiver, &s_receiver) != WL_BULK_OK) return WL_ERR_INVALID_ARG;
        return wl_set_sink(&s_host, [](void* s_context, wl_io_token_t, const std::uint8_t* s_data,
                                       std::size_t s_size) -> wl_sink_result_t {
            auto& s_self = *static_cast<PeerTransport*>(s_context);
            std::size_t s_accepted{};
            return wl_feed_bytes(&s_self.m_peer, s_data, s_size, &s_accepted) == WL_OK &&
                   s_accepted == s_size ? WL_SINK_SENT : WL_SINK_FAILED;
        }, this);
    }
    std::uint32_t wirelinkDeadlineHint(wl_time_ms_t) const noexcept override { return 1; }
    int serviceWirelink() noexcept override {
        wl_event_t s_event{};
        while (wl_poll(&m_peer, now(), &s_event) == WL_OK) {
            if (s_event.type == WL_EVT_RELIABLE_RX || s_event.type == WL_EVT_UNRELIABLE_RX) {
                receive(s_event); wl_event_release(&m_peer, &s_event);
            } else if (s_event.handle) {
                wl_tx_result_t s_result{}; (void)wl_tx_take(&m_peer, s_event.handle, &s_result);
            }
        }
        if (!m_responses.empty()) {
            const auto& s_reply = m_responses.front(); wl_tx_handle_t s_handle{};
            if (wl_send_reliable(&m_peer, s_reply.m_id, s_reply.m_data.data(),
                s_reply.m_data.size(), now(), &s_handle) == WL_OK) m_responses.pop_front();
        }
        if (m_state->m_streaming && now() - m_stream_ms >= 4) {
            m_stream_ms = now();
            underwater_arm_status_t s_wire{};
            s_wire.has_q = s_wire.has_dq = s_wire.has_tau = s_wire.has_motor_info = s_wire.has_feedback_age_ms = true;
            for (int s_i = 0; s_i < 10; ++s_i) {
                s_wire.q[s_i] = s_i * 0.125f; s_wire.dq[s_i] = -s_i * 0.25f; s_wire.tau[s_i] = s_i * 0.5f;
                s_wire.motor_info[s_i] = (1U << 8) | (25+s_i); s_wire.feedback_age_ms[s_i] = s_i;
            }
            s_wire.has_fresh_mask = s_wire.has_enabled_mask = s_wire.has_available_mask = true;
            s_wire.fresh_mask = s_wire.enabled_mask = s_wire.available_mask = 0x3ff;
            s_wire.has_mode = s_wire.has_kind = s_wire.has_sequence = s_wire.has_timestamp_us = true;
            s_wire.mode = m_state->m_mode; s_wire.kind = UNDERWATER_CONTROL_MIT;
            s_wire.sequence = ++m_state->m_stream_sequence; s_wire.timestamp_us = now()*1000ULL;
            s_wire.has_control_session = s_wire.has_applied_command_sequence = s_wire.has_last_sdk_timestamp_us = true;
            s_wire.has_applied_mask = s_wire.has_command_received = s_wire.has_control_stop_reason = true;
            s_wire.has_commands_received = s_wire.has_commands_overwritten = s_wire.has_commands_rejected = true;
            s_wire.has_command_reject_reason = true;
            {
                std::lock_guard s_lock(m_state->m_stream_mutex);
                s_wire.control_session = m_state->m_control_session;
                s_wire.applied_command_sequence = m_state->m_command.sequence;
                s_wire.last_sdk_timestamp_us = m_state->m_command.sdk_timestamp_us;
                s_wire.command_received = m_state->m_command_count != 0;
                s_wire.commands_received = m_state->m_command_count;
                s_wire.applied_mask = s_wire.command_received ? 0x3ff : 0;
            }
            std::array<std::uint8_t, 512> s_bytes{}; std::size_t s_size{};
            require(underwater_arm_status_encode(&s_wire, s_bytes.data(), s_bytes.size(), &s_size) == WL_CODEC_OK, "stream encode");
            (void)wl_send_unreliable(&m_peer, UNDERWATER_ARM_STATUS_MESSAGE_ID, s_bytes.data(), s_size);
        }
        (void)wl_bulk_receiver_poll(&m_receiver, now());
        wl_bulk_receiver_status_view_t s_view{};
        if (wl_bulk_receiver_status_acquire(&m_receiver, &s_view) == WL_BULK_OK) {
            bulk_status_t s_message{};
            s_message.has_transfer_id = s_message.has_phase = s_message.has_code = true;
            s_message.has_next_offset = s_message.has_accepted_chunk_size = true;
            s_message.transfer_id = s_view.status.transfer_id; s_message.phase = s_view.status.phase;
            s_message.code = s_view.status.code; s_message.next_offset = s_view.status.next_offset;
            s_message.accepted_chunk_size = s_view.status.accepted_chunk_size;
            if (fci_device_bulk_status_send(&m_peer, &s_message, WL_DELIVERY_UNRELIABLE, now()).domain == FCI_DEVICE_SEND_OK)
                (void)wl_bulk_receiver_status_release(&m_receiver, &s_view);
            else (void)wl_bulk_receiver_status_defer(&m_receiver, &s_view);
        }
        return WL_OK;
    }

private:
    static std::uint32_t now() {
        return static_cast<std::uint32_t>(std::chrono::duration_cast<Ms>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    using Ms = std::chrono::milliseconds;
    struct Reply { std::uint16_t m_id; std::vector<std::uint8_t> m_data; };
    template <typename T, typename Encode>
    void reply(std::uint16_t s_id, const T& s_value, Encode s_encode) {
        Reply s_reply{s_id, std::vector<std::uint8_t>(1024)};
        std::size_t s_size{};
        require(s_encode(&s_value, s_reply.m_data.data(), s_reply.m_data.size(), &s_size) == WL_CODEC_OK, "peer encode");
        s_reply.m_data.resize(s_size); m_responses.push_back(std::move(s_reply));
    }
    template <typename Response, typename Request>
    void replySettings(const Request& s_request, std::uint16_t s_id, bool s_defaults) {
        Response s_reply{};
        s_reply.has_operation_id = s_reply.has_status = s_reply.has_result = true;
        s_reply.has_snapshot = s_reply.has_reason = true;
        s_reply.operation_id = s_request.operation_id; s_reply.session = s_request.session;
        s_reply.result = std::is_same_v<Response, SetUnderwaterSettingsResponse> && m_state->m_upgrading
            ? UNDERWATER_SETTINGS_BUSY : m_state->m_settings_result.load();
        auto& s_snapshot = s_reply.snapshot;
        s_snapshot.has_settings = s_snapshot.has_revision = s_snapshot.has_source = s_snapshot.has_storage_error = true;
        s_snapshot.settings = s_defaults ? settingsFixture() : m_state->m_settings;
        s_snapshot.revision = s_defaults ? 0 : m_state->m_revision;
        s_snapshot.source = s_defaults ? UNDERWATER_SETTINGS_SOURCE_DEFAULTS : m_state->m_settings_source.load();
        s_snapshot.storage_error = m_state->m_storage_error;
        if (s_reply.result == UNDERWATER_SETTINGS_STORAGE_FAILED) {
            static const std::string s_reason(95, 'S');
            s_reply.reason = {s_reason.data(), s_reason.size()};
        } else if (s_reply.result == UNDERWATER_SETTINGS_BUSY) s_reply.reason = {"upgrade active", 14};
        else s_reply.reason = {"settings snapshot", 17};
        reply(s_id, s_reply, Response::encode);
    }
    std::uint32_t m_stream_ms{};
    underwater_status_t status() {
        underwater_status_t s_info{};
        s_info.has_mode = s_info.has_uptime_ms = s_info.has_enabling_arms = s_info.has_gravity_arms = true;
        s_info.has_master_available_mask = s_info.has_slave_available_mask = true;
        s_info.has_master_enabled_mask = s_info.has_slave_enabled_mask = true;
        s_info.has_master_disable_pending_mask = s_info.has_slave_disable_pending_mask = true;
        s_info.has_upgrading = s_info.has_reason = true;
        s_info.has_master_disabled_mask = s_info.has_slave_disabled_mask = m_state->m_disabled_masks;
        s_info.master_disabled_mask = 3; s_info.slave_disabled_mask = 15;
        s_info.has_control_session = true;
        s_info.control_session = m_state->m_control_session;
        s_info.mode = m_state->m_mode; s_info.uptime_ms = 0xf1234567;
        s_info.enabling_arms = s_info.mode == UNDERWATER_MODE_GRAVITY ? 1 : 0;
        s_info.gravity_arms = s_info.mode == UNDERWATER_MODE_RETRACT ? 1 : 0;
        s_info.master_available_mask = 31; s_info.slave_available_mask = 15;
        s_info.master_enabled_mask = s_info.mode == UNDERWATER_MODE_RETRACT ? 31 : 0;
        s_info.slave_enabled_mask = 0;
        s_info.master_disable_pending_mask = s_info.mode == UNDERWATER_MODE_DAMP ? 3 : 0;
        s_info.slave_disable_pending_mask = s_info.mode == UNDERWATER_MODE_DAMP ? 15 : 0;
        s_info.upgrading = m_state->m_upgrading;
        if (s_info.upgrading) s_info.reason = {"upgrade active", 14};
        else s_info.reason = {"waiting for slave gripper", 25};
        return s_info;
    }
    void receive(const wl_event_t& s_event) {
        switch (s_event.message_id) {
        case ZEROING_REQUEST_MESSAGE_ID: {
            using Request = test_rpc::Envelope<zeroing_request_t, true, zeroing_request_encode, zeroing_request_decode>;
            using Response = test_rpc::Envelope<zeroing_response_t, false, zeroing_response_encode, zeroing_response_decode>;
            Request s_request{};
            require(Request::decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "Zeroing request decode");
            ++m_state->m_zeroing_count;
            Response s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_result = s_reply.has_info = true;
            s_reply.operation_id = s_request.operation_id; s_reply.session = s_request.session;
            s_reply.result = m_state->m_upgrading ? UNDERWATER_RESULT_BUSY :
                m_state->m_mode == UNDERWATER_MODE_DAMP ? UNDERWATER_RESULT_OK : UNDERWATER_RESULT_INVALID_STATE;
            s_reply.info = status();
            reply(ZEROING_RESPONSE_MESSAGE_ID, s_reply, Response::encode);
            break;
        }
        case UNDERWATER_ARM_COMMAND_MESSAGE_ID: {
            std::lock_guard s_lock(m_state->m_stream_mutex);
            require(underwater_arm_command_decode(s_event.payload, s_event.payload_len, &m_state->m_command) == WL_CODEC_OK, "ten-axis command decode");
            require(m_state->m_command.control_session == m_state->m_control_session, "SDK session on command");
            ++m_state->m_command_count;
            break;
        }
        case GET_UNDERWATER_SETTINGS_REQUEST_MESSAGE_ID: {
            GetUnderwaterSettingsRequest s_request{};
            require(GetUnderwaterSettingsRequest::decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "settings get decode");
            replySettings<GetUnderwaterSettingsResponse>(s_request, GET_UNDERWATER_SETTINGS_RESPONSE_MESSAGE_ID, false);
            break;
        }
        case GET_UNDERWATER_DEFAULT_SETTINGS_REQUEST_MESSAGE_ID: {
            GetUnderwaterDefaultSettingsRequest s_request{};
            require(GetUnderwaterDefaultSettingsRequest::decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "defaults decode");
            replySettings<GetUnderwaterDefaultSettingsResponse>(s_request, GET_UNDERWATER_DEFAULT_SETTINGS_RESPONSE_MESSAGE_ID, true);
            break;
        }
        case SET_UNDERWATER_SETTINGS_REQUEST_MESSAGE_ID: {
            ++m_state->m_settings_count;
            SetUnderwaterSettingsRequest s_request{};
            require(SetUnderwaterSettingsRequest::decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "settings set decode");
            if (!m_state->m_upgrading && m_state->m_settings_result == 0) {
                m_state->m_settings = s_request.settings;
                ++m_state->m_revision;
                m_state->m_settings_source = UNDERWATER_SETTINGS_SOURCE_STORED;
            }
            replySettings<SetUnderwaterSettingsResponse>(s_request, SET_UNDERWATER_SETTINGS_RESPONSE_MESSAGE_ID, false);
            break;
        }
        case GET_UNDERWATER_STATUS_REQUEST_MESSAGE_ID: {
            ++m_state->m_get_count;
            if (m_state->m_silent) break;
            GetRequest s_request{};
            require(GetRequest::decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "get decode");
            GetResponse s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_info = s_reply.has_result = true;
            s_reply.operation_id = s_request.operation_id; s_reply.session = s_request.session;
            s_reply.info = status();
            reply(GET_UNDERWATER_STATUS_RESPONSE_MESSAGE_ID, s_reply, GetResponse::encode);
            break;
        }
        case SET_UNDERWATER_MODE_REQUEST_MESSAGE_ID: {
            ++m_state->m_set_count;
            SetRequest s_request{};
            require(SetRequest::decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "set decode");
            SetResponse s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_info = s_reply.has_result = true;
            s_reply.operation_id = s_request.operation_id; s_reply.session = s_request.session;
            s_reply.status = m_state->m_rejection;
            s_reply.result = m_state->m_upgrading && s_request.mode != UNDERWATER_MODE_DAMP
                ? UNDERWATER_RESULT_BUSY : m_state->m_business_status.load();
            if (s_reply.result == UNDERWATER_RESULT_OK && !s_reply.status)
                m_state->m_mode = s_request.mode == UNDERWATER_MODE_TELEOP ? UNDERWATER_MODE_RETRACT : s_request.mode;
            if (s_request.mode == UNDERWATER_MODE_CONTROL && s_reply.result == UNDERWATER_RESULT_OK) ++m_state->m_control_session;
            s_reply.info = status();
            reply(SET_UNDERWATER_MODE_RESPONSE_MESSAGE_ID, s_reply, SetResponse::encode);
            break;
        }
        case GET_BOOT_STATUS_REQUEST_MESSAGE_ID: {
            get_boot_status_request_t s_request{};
            require(get_boot_status_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "boot decode");
            get_boot_status_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_info = true;
            s_reply.operation_id = s_request.operation_id;
            auto& s_info = s_reply.info;
            s_info.has_protocol_version = s_info.has_boot_state = s_info.has_recovery_reason = true;
            s_info.has_active_slot = s_info.has_confirmed_slot = s_info.has_pending_slot = true;
            s_info.has_write_align = s_info.has_max_chunk_size = true;
            s_info.has_slot0_address = s_info.has_slot0_size = s_info.has_slot1_address = s_info.has_slot1_size = s_info.has_bytes_written = true;
            s_info.protocol_version = 1; s_info.write_align = 1; s_info.max_chunk_size = 128;
            s_info.bytes_written = m_state->m_written; s_info.active_slot = s_info.confirmed_slot = SLOT_PRIMARY;
            s_info.slot0_address = 0x08020000; s_info.slot1_address = 0x08080000;
            s_info.slot0_size = s_info.slot1_size = 393216;
            reply(GET_BOOT_STATUS_RESPONSE_MESSAGE_ID, s_reply, get_boot_status_response_encode);
            break;
        }
        case GET_DEVICE_INFO_REQUEST_MESSAGE_ID: {
            test_rpc::get_device_info_request_t s_request{};
            require(test_rpc::get_device_info_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "info decode");
            test_rpc::get_device_info_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_info = true;
            s_reply.operation_id = s_request.operation_id; s_reply.session = s_request.session;
            auto& s_info = s_reply.info;
            s_info.has_protocol_version = s_info.has_firmware_version = true;
            s_info.protocol_version = {.has_major = true, .major = 0, .has_minor = true, .minor = 0, .has_patch = true, .patch = 1};
            s_info.firmware_version = {.has_major = true, .major = 0, .has_minor = true, .minor = 9, .has_patch = true, .patch = 0};
            s_info.has_board_name = s_info.has_custom_name = s_info.has_serial = s_info.has_firmware_type = true;
            s_info.board_name = {"ragtime_florid", 14}; s_info.custom_name = {"Underwater", 10};
            s_info.serial = {"TEST-H7", 7}; s_info.firmware_type = FIRMWARE_STANDARD_ARM;
            reply(GET_DEVICE_INFO_RESPONSE_MESSAGE_ID, s_reply, test_rpc::get_device_info_response_encode);
            break;
        }
        case START_UPGRADE_REQUEST_MESSAGE_ID: {
            ++m_state->m_start_count;
            start_upgrade_request_t s_request{};
            require(start_upgrade_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "start decode");
            start_upgrade_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_transfer_id = true;
            s_reply.has_write_align = s_reply.has_max_chunk_size = s_reply.has_next_offset = true;
            s_reply.operation_id = s_request.operation_id; s_reply.status = m_state->m_fail_start ? -11 : 0;
            s_reply.transfer_id = ++m_transfer; s_reply.write_align = 1; s_reply.max_chunk_size = 128;
            if (!s_reply.status) { m_state->m_upgrading = true; m_state->m_mode = UNDERWATER_MODE_DAMP; }
            (void)wl_bulk_receiver_reset(&m_receiver);
            reply(START_UPGRADE_RESPONSE_MESSAGE_ID, s_reply, start_upgrade_response_encode);
            break;
        }
        case REBOOT_REQUEST_MESSAGE_ID: {
            ++m_state->m_reboot_count;
            reboot_request_t s_request{};
            require(reboot_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "reboot decode");
            reboot_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_mode = true;
            s_reply.operation_id = s_request.operation_id; s_reply.mode = s_request.mode; s_reply.status = -16;
            reply(REBOOT_RESPONSE_MESSAGE_ID, s_reply, reboot_response_encode);
            break;
        }
        case BULK_BEGIN_MESSAGE_ID: {
            bulk_begin_t s_msg{}; require(bulk_begin_decode(s_event.payload, s_event.payload_len, &s_msg) == WL_CODEC_OK, "begin decode");
            wl_bulk_descriptor_t s_descriptor{s_msg.transfer_id, s_msg.total_length, s_msg.requested_chunk_size, s_msg.object_crc32c};
            (void)wl_bulk_receiver_on_begin(&m_receiver, &s_descriptor, now()); break;
        }
        case BULK_CHUNK_MESSAGE_ID: {
            bulk_chunk_t s_msg{}; require(bulk_chunk_decode(s_event.payload, s_event.payload_len, &s_msg) == WL_CODEC_OK, "chunk decode");
            wl_bulk_chunk_t s_chunk{s_msg.transfer_id, s_msg.offset, s_msg.data.data, s_msg.data.length};
            (void)wl_bulk_receiver_on_chunk(&m_receiver, &s_chunk, now()); break;
        }
        case BULK_END_MESSAGE_ID: {
            bulk_end_t s_msg{}; require(bulk_end_decode(s_event.payload, s_event.payload_len, &s_msg) == WL_CODEC_OK, "end decode");
            (void)wl_bulk_receiver_on_end(&m_receiver, s_msg.transfer_id, s_msg.total_length, s_msg.object_crc32c, now()); break;
        }
        case BULK_ABORT_MESSAGE_ID: {
            bulk_abort_t s_msg{}; require(bulk_abort_decode(s_event.payload, s_event.payload_len, &s_msg) == WL_CODEC_OK, "abort decode");
            (void)wl_bulk_receiver_on_abort(&m_receiver, s_msg.transfer_id, s_msg.reason, now()); break;
        }
        default: ++m_state->m_other_count; break;
        }
    }
    std::shared_ptr<State> m_state;
    wl_ctx_t* m_host{};
    wl_ctx_t m_peer{};
    wl_bulk_receiver_t m_receiver{};
    std::uint32_t m_transfer{};
    std::array<std::uint8_t, 2048> m_payload{};
    std::array<std::uint8_t, 2200> m_tx{}, m_fallback{};
    std::array<std::uint8_t, 8192> m_rx{};
    std::array<std::uint8_t, 64> m_control{};
    std::deque<Reply> m_responses;
};

void testModeAndLifecycle() {
    auto s_state = std::make_shared<State>();
    auto s_connected = detail::UnderwaterTeleopTestAccess::connect(std::make_unique<PeerTransport>(s_state));
    require(bool(s_connected), "connect and status probe");
    auto s_device = std::move(s_connected.m_device);
    require(s_state->m_set_count == 0 && s_state->m_other_count == 0, "connect must not enable or acquire a lease");
    auto s_reply = s_device->status();
    require(bool(s_reply) && s_reply.m_info->m_uptime_ms == 0xf1234567 &&
            s_reply.m_info->m_master_available_mask == 31 && s_reply.m_info->m_slave_available_mask == 15 &&
            s_reply.m_info->m_slave_disable_pending_mask == 15 &&
            s_reply.m_info->m_reason == "waiting for slave gripper", "real status snapshot decoded");
    s_reply = s_device->enable();
    require(bool(s_reply) && s_reply.m_info->m_mode == UnderwaterMode::kGravity &&
            s_reply.m_info->m_enabling_arms == 1 && s_reply.m_info->m_master_enabled_mask == 0,
            "accepted enable is not physical completion");
    s_reply = s_device->enterTeleop();
    require(bool(s_reply) && s_reply.m_info->m_mode == UnderwaterMode::kRetract &&
            s_reply.m_info->m_gravity_arms == 1 && s_reply.m_info->m_master_enabled_mask == 31,
            "enter teleop returns actual Retract phase");
    for (int s_code = 1; s_code <= 5; ++s_code) {
        s_state->m_business_status = s_code;
        s_reply = s_device->enable();
        require(!s_reply && s_reply.m_error == UnderwaterRpcError::kDeviceRejected &&
                s_reply.m_device_status == s_code && s_reply.m_info &&
                s_reply.m_info->m_mode == UnderwaterMode::kRetract && !s_reply.m_info->m_reason.empty(),
                "business rejection preserves result and snapshot");
    }
    s_state->m_business_status = 0; s_state->m_rejection = -123;
    s_reply = s_device->enable();
    require(!s_reply && s_reply.m_error == UnderwaterRpcError::kDeviceRejected &&
            s_reply.m_device_status == -123 && !s_reply.m_info, "managed rejection remains distinct from business snapshot");
    s_state->m_rejection = 0;
    s_reply = s_device->disable();
    require(bool(s_reply) && s_reply.m_info->m_mode == UnderwaterMode::kDamp &&
            s_reply.m_info->m_master_disable_pending_mask == 3, "disable reports pending confirmations");
    s_state->m_silent = true;
    require(s_device->status(20ms).m_error == UnderwaterRpcError::kTimeout, "missing response gives timeout");
    std::this_thread::sleep_for(30ms); s_state->m_silent = false;
    require(bool(s_device->status()), "timeout request reaped for next query");
    const int s_writes = s_state->m_set_count;
    s_device->close();
    require(s_state->m_set_count == s_writes && s_state->m_other_count == 0, "close sends no disable/lease/motor RPCs");
    require(s_device->enable().m_error == UnderwaterRpcError::kNotConnected, "closed handle explicit status");
    require(!UnderwaterTeleop::connect("not-a-uri"), "bad URI returns error");
    require(!UnderwaterTeleop::connect(DeviceDescriptor{}), "empty descriptor must not select a random device");
}

void testUpgradeReuse() {
    auto s_state = std::make_shared<State>();
    auto s_connected = detail::UnderwaterTeleopTestAccess::connect(std::make_unique<PeerTransport>(s_state));
    require(bool(s_connected), "upgrade connection");
    auto& s_device = *s_connected.m_device;
    const auto s_identity = s_device.deviceInfo();
    require(bool(s_identity) && s_identity.m_info->m_serial_number == "TEST-H7" &&
            s_identity.m_info->m_firmware_version.m_minor == 9, "device info shares existing endpoint");
    const auto s_same_version = s_device.rebootAndWait(Version{0, 9, 0});
    require(s_same_version.m_error == FirmwareUpdateError::kInvalidArgument && s_state->m_reboot_count == 0,
            "existing verification semantics reject equal version before reboot");
    const auto s_boot = s_device.bootStatus();
    require(s_boot.m_error == FirmwareUpdateError::kNone && s_boot.m_slot0_address == 0x08020000 &&
            s_boot.m_max_chunk_size == 128, "boot query reuses upgrade RPC");
    std::vector<std::uint8_t> s_image(777);
    for (std::size_t s_i = 0; s_i < s_image.size(); ++s_i) s_image[s_i] = static_cast<std::uint8_t>(s_i * 13);
    s_state->m_hold_chunk = true;
    require(s_device.startFirmwareUpload(s_image) == FirmwareUpdateError::kNone, "upload queued");
    const auto s_deadline = std::chrono::steady_clock::now() + 2s;
    while (!s_state->m_upgrading && std::chrono::steady_clock::now() < s_deadline) std::this_thread::sleep_for(1ms);
    require(s_state->m_upgrading, "upgrade started");
    auto s_mode = s_device.status();
    require(bool(s_mode) && s_mode.m_info->m_upgrading, "status remains available during upload");
    s_mode = s_device.enable();
    require(!s_mode && s_mode.m_device_status == UNDERWATER_RESULT_BUSY && s_mode.m_info &&
            s_mode.m_info->m_reason == "upgrade active", "Busy comes from firmware with snapshot during upgrade");
    require(bool(s_device.disable()), "Damp request reaches firmware during upload");
    const auto s_settings = s_device.setSettings(UnderwaterSettings{});
    require(!s_settings && s_settings.m_device_status == UNDERWATER_SETTINGS_BUSY &&
            s_settings.m_reason == "upgrade active" && s_settings.m_snapshot && s_state->m_settings_count == 1,
            "settings write reaches firmware during upgrade and retains its Busy snapshot");
    s_state->m_hold_chunk = false;
    while (s_device.firmwareProgress().active() && std::chrono::steady_clock::now() < s_deadline)
        std::this_thread::sleep_for(1ms);
    const auto s_uploaded = s_device.firmwareProgress();
    require(s_uploaded.m_state == FirmwareUploadState::kCompleted &&
            s_uploaded.m_acknowledged_bytes == s_image.size() &&
            std::equal(s_image.begin(), s_image.end(), s_state->m_image.begin()), "real bulk image uploaded with CRC");
    require(s_state->m_reboot_count == 0, "upload does not reboot automatically");
    const auto s_reboot = s_device.reboot();
    require(s_reboot.m_error == FirmwareUpdateError::kDeviceRejected && s_reboot.m_device_status == -16 &&
            !s_reboot.m_request_accepted, "reboot propagates firmware rejection");
    s_state->m_fail_start = true;
    const auto s_updated = s_device.updateFirmware(s_image, Version{1, 2, 3});
    require(!s_updated && s_updated.m_upload.m_error == FirmwareUpdateError::kDeviceRejected &&
            s_updated.m_upload.m_device_status == -11 && !s_updated.m_install && s_state->m_reboot_count == 1,
            "update stops on upload rejection and never reboots");
}

void testSettings() {
    auto s_state = std::make_shared<State>();
    auto s_connection = detail::UnderwaterTeleopTestAccess::connect(std::make_unique<PeerTransport>(s_state));
    require(bool(s_connection), "settings connection");
    auto& s_device = *s_connection.m_device;
    auto s_mode = s_device.status();
    require(!s_mode.m_info->m_master_disabled_mask && !s_mode.m_info->m_slave_disabled_mask,
            "old firmware missing disabled masks are not fabricated");
    s_state->m_disabled_masks = true;
    s_mode = s_device.status();
    require(s_mode.m_info->m_master_disabled_mask == 3 && s_mode.m_info->m_slave_disabled_mask == 15,
            "fresh disabled masks survive protocol and SDK conversion");
    auto s_reply = s_device.settings();
    require(bool(s_reply) && s_reply.m_snapshot->m_revision == 7 &&
            s_reply.m_snapshot->m_source == UnderwaterSettingsSource::kDefaults,
            "settings authoritative metadata read");
    checkSettings(s_reply.m_snapshot->m_settings, settingsFixture());
    auto s_settings = s_reply.m_snapshot->m_settings;
    s_settings.m_buzzer_alarm_enabled = false;
    s_settings.m_angle_scale[2] = -1.75f;
    s_settings.m_gravity_scale[5] = -0.5f;
    s_settings.m_force_gain[2] = 0.333f;
    s_settings.m_friction_coulomb_positive_nm[9] = 0.125f;
    s_state->m_mode = UNDERWATER_MODE_TELEOP;
    s_reply = s_device.setSettings(s_settings);
    require(bool(s_reply) && s_reply.m_snapshot->m_revision == 8 &&
            s_reply.m_snapshot->m_source == UnderwaterSettingsSource::kStored &&
            s_state->m_set_count == 0 && s_state->m_mode == UNDERWATER_MODE_TELEOP,
            "SDK setting write neither changes nor gates mode");
    require(s_state->m_settings.has_buzzer_alarm_enabled && !s_state->m_settings.buzzer_alarm_enabled,
            "muting is sent explicitly, not omitted");
    checkSettings(s_settings, s_state->m_settings);
    checkSettings(s_reply.m_snapshot->m_settings, s_state->m_settings);
    auto s_defaults = s_device.defaultSettings();
    require(bool(s_defaults) && s_defaults.m_snapshot->m_source == UnderwaterSettingsSource::kDefaults &&
            s_state->m_revision == 8 && s_state->m_settings_count == 1,
            "reading defaults does not write or advance revision");
    checkSettings(s_defaults.m_snapshot->m_settings, settingsFixture());
    for (int s_code = 1; s_code <= 3; ++s_code) {
        s_state->m_settings_result = s_code;
        s_reply = s_device.setSettings(UnderwaterSettings{});
        require(!s_reply && s_reply.m_error == UnderwaterRpcError::kDeviceRejected &&
                s_reply.m_device_status == s_code && s_reply.m_snapshot->m_revision == 8,
                "negative settings result preserves actual revision/snapshot");
        checkSettings(s_reply.m_snapshot->m_settings, s_state->m_settings);
        if (s_code == UNDERWATER_SETTINGS_STORAGE_FAILED)
            require(s_reply.m_reason == std::string(95, 'S'), "full 95-byte rejection reason retained");
    }
    s_state->m_settings_result = 0;
    s_state->m_settings_source = UNDERWATER_SETTINGS_SOURCE_CORRUPT;
    s_state->m_storage_error = -5;
    s_reply = s_device.settings();
    require(bool(s_reply) && s_reply.m_snapshot->m_source == UnderwaterSettingsSource::kCorrupt &&
            s_reply.m_snapshot->m_storage_error == -5, "corrupt-storage metadata does not hide usable runtime settings");
    s_state->m_silent = true;
    require(s_device.status(20ms).m_error == UnderwaterRpcError::kTimeout, "mode query times out before settings query");
    std::this_thread::sleep_for(30ms); s_state->m_silent = false;
    require(bool(s_device.settings()), "settings query reaps timed-out status operation");
    s_device.close();
    require(s_device.settings().m_error == UnderwaterRpcError::kNotConnected &&
            s_device.setSettings(s_settings).m_error == UnderwaterRpcError::kNotConnected,
            "settings on closed SDK returns explicit error");
}
void testZeroing() {
    auto s_state = std::make_shared<State>();
    auto s_connection = florid::detail::UnderwaterTeleopTestAccess::connect(std::make_unique<PeerTransport>(s_state));
    require(bool(s_connection), "Zeroing connect");
    auto& s_device = *s_connection.m_device;
    require(bool(s_device.zeroing()) && s_state->m_zeroing_count == 1, "Zeroing RPC in Damp");
    require(bool(s_device.enable()), "enable before rejected calibration");
    const auto s_result = s_device.zeroing();
    require(!s_result && s_result.m_device_status == UNDERWATER_RESULT_INVALID_STATE &&
        s_result.m_info->m_mode == UnderwaterMode::kGravity, "Zeroing rejection carries actual mode");
    require(s_state->m_mode == UNDERWATER_MODE_GRAVITY, "SDK does not implicitly disable");
    s_state->m_upgrading = true;
    require(s_device.zeroing().m_device_status == UNDERWATER_RESULT_BUSY, "upgrade Zeroing Busy");
    s_device.close();
    require(s_device.zeroing().m_error == UnderwaterRpcError::kNotConnected, "closed Zeroing handle");
}

void testControlStreaming() {
    auto s_state = std::make_shared<State>();
    auto s_connection = florid::detail::UnderwaterTeleopTestAccess::connect(std::make_unique<PeerTransport>(s_state));
    require(bool(s_connection), "connect stream SDK");
    auto& s_device = *s_connection.m_device;
    UnderwaterArmCommand s_command{};
    require(!s_device.writeArmCommand(s_command), "Control RPC supplies the command session");
    const auto s_entered = s_device.enterControl();
    require(bool(s_entered) && s_entered.m_info->m_mode == UnderwaterMode::kControl &&
        s_entered.m_info->m_control_session != 0, "Control RPC carries session");
    s_state->m_streaming = true;
    for (int s_i = 0; s_i < 10; ++s_i) {
        s_command.m_q[s_i] = -0.1f*s_i; s_command.m_dq[s_i] = 0.02f*s_i;
        s_command.m_kp[s_i] = s_i+5; s_command.m_kd[s_i] = 0.5f; s_command.m_tau[s_i] = s_i*0.25f;
    }
    auto s_deadline = std::chrono::steady_clock::now();
    std::uint32_t s_last{};
    for (int s_i = 0; s_i < 250; ++s_i) {
        s_command.m_kind = s_i < 125 ? UnderwaterControlKind::kMIT : UnderwaterControlKind::kMitWithGravity;
        const auto s_result = s_device.writeArmCommand(s_command);
        require(bool(s_result), "250 Hz command admitted"); s_last = s_result.m_sequence;
        s_deadline += 4ms; std::this_thread::sleep_until(s_deadline);
    }
    std::this_thread::sleep_for(20ms);
    const auto s_status = s_device.readArmStatus();
    require(s_status.has_value() && s_status->m_sequence >= 100, "continuous ten-axis latest telemetry");
    require(s_status->m_applied_command_sequence == s_last && s_status->m_applied_mask == 0x3ff,
            "command sequence echo through actual wire codec");
    for (int s_i = 0; s_i < 10; ++s_i) {
        require(s_status->m_q[s_i] == s_i*0.125f && s_status->m_temperature_c[s_i] == 25+s_i &&
            s_status->m_motor_status[s_i] == 1 && s_status->m_feedback_age_ms[s_i] == unsigned(s_i), "ten-axis order and packed motor metadata");
    }
    {
        std::lock_guard s_lock(s_state->m_stream_mutex);
        require(s_state->m_command_count >= 100, "250 Hz producer reaches peer without RPC per frame");
        const auto& s_wire = s_state->m_command;
        require(s_wire.kind == UNDERWATER_CONTROL_MIT_WITH_GRAVITY, "second MIT variant");
        require(std::equal(s_command.m_q.begin(), s_command.m_q.end(), s_wire.q) &&
                std::equal(s_command.m_tau.begin(), s_command.m_tau.end(), s_wire.tau), "all ten commands copied");
        require(s_wire.sdk_timestamp_us != 0, "SDK timestamps generated automatically");
    }
    require(bool(s_device.disable()), "exit Control RPC");
    require(!s_device.writeArmCommand(s_command), "disable invalidates local command session");
    s_device.close();
}

}
int main() {
    try { testModeAndLifecycle(); testUpgradeReuse(); testSettings(); testControlStreaming(); testZeroing(); std::puts("underwater SDK protocol/lifecycle/upgrade/settings tests passed"); }
    catch (const std::exception& s_error) { std::fprintf(stderr, "%s\n", s_error.what()); return 1; }
}
