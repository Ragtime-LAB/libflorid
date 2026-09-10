#include "../protocol/tests/support/managed_rpc_wire.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"
#include "florid/FirmwareUpdater.hpp"
#include <wirelink/crc.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <deque>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;
using namespace florid;
using namespace florid::detail;

namespace {
void require(bool s_ok, const char* s_message) { if (!s_ok) throw std::runtime_error(s_message); }

struct TestTransport : Transport {
    wl_sink_fn m_sink{};
    void* m_context{};
    std::atomic<TransportConnectionState> m_state{TransportConnectionState::kConnected};
    bool send(const std::uint8_t*, std::size_t) override { return false; }
    void setReceiveCallback(ReceiveFunctor, void*) override {}
    bool usesDirectWirelink() const noexcept override { return true; }
    int attachWirelink(wl_ctx_t& s_link, WakeFunctor, void*) noexcept override {
        return wl_set_sink(&s_link, [](void* s_context, wl_io_token_t s_token,
            const std::uint8_t* s_data, std::size_t s_size) -> wl_sink_result_t {
            auto& s_self = *static_cast<TestTransport*>(s_context);
            return s_self.m_state == TransportConnectionState::kConnected
                ? s_self.m_sink(s_self.m_context, s_token, s_data, s_size) : WL_SINK_FAILED;
        }, this);
    }
    int serviceWirelink() noexcept override { return WL_OK; }
    void quiesceWirelink() noexcept override { m_state = TransportConnectionState::kClosed; }
    TransportConnectionState connectionState() const noexcept override { return m_state.load(); }
};

// Independent wire peer. Uses the real bulk receiver and raw FCI codecs;
// never feeds two contexts with the same input or drives the host owner.
struct Peer {
    TestTransport m_transport;
    FciWirelinkEndpoint m_host;
    wl_ctx_t m_link{};
    wl_bulk_receiver_t m_receiver{};
    std::array<std::uint8_t, 2048> m_payload{};
    std::array<std::uint8_t, 2200> m_tx{}, m_fallback{};
    std::array<std::uint8_t, 8192> m_rx{};
    std::array<std::uint8_t, 64> m_control{};
    std::array<std::uint8_t, 65536> m_ram{};
    std::uint64_t m_written{};
    std::uint32_t m_transfer{}, m_crc{}, m_finishes{}, m_writes{};
    bool m_busy{}, m_drop_status{true};
    std::atomic<bool> m_stop{}, m_silent_start{}, m_hold_chunk{};
    std::atomic<bool> m_silent_reboot{};
    std::atomic<std::int32_t> m_reboot_status{};
    std::atomic<bool> m_depart_on_reboot{true}, m_silent_info{}, m_wrong_identity{}, m_wrong_protocol{};
    std::atomic<bool> m_confirm{true}, m_reset_without_reply{};
    std::atomic<std::uint32_t> m_next_version{2}, m_reboot_requests{}, m_reboot_acks{};
    std::uint32_t m_version{1};
    std::uint64_t m_session{199};
    bool m_reset_requested{};
    wl_tx_handle_t m_reboot_handle{};
    std::mutex m_link_reset_mutex;
    std::thread m_thread;
    struct Response { std::uint16_t m_id{}; std::vector<std::uint8_t> m_bytes; };
    std::deque<Response> m_responses;

    static std::uint32_t s_now() {
        return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    void s_initLink() {
        std::lock_guard s_lock(m_link_reset_mutex);
        wl_config_t s_config{};
        s_config.max_payload_len = m_payload.size(); s_config.envelope = WL_ENVELOPE_COBS_STREAM;
        s_config.integrity = WL_INTEGRITY_NONE; s_config.session_id = m_session;
        s_config.max_retries = 2; s_config.ack_timeout_ms = 20; s_config.max_transmission_unit = m_tx.size();
        wl_storage_t s_storage{m_payload.data(), m_payload.size(), m_tx.data(), m_tx.size(),
            m_control.data(), m_control.size(), m_rx.data(), m_rx.size(), m_fallback.data(), m_fallback.size()};
        require(wl_init(&m_link, &s_config, &s_storage) == WL_OK, "peer init");
        require(wl_set_sink(&m_link, [](void* s_context, wl_io_token_t, const std::uint8_t* s_data, std::size_t s_size) -> wl_sink_result_t {
            auto& s_self = *static_cast<Peer*>(s_context);
            // Fragment every frame so direct bytes have to survive real COBS RX assembly.
            for (std::size_t s_offset = 0; s_offset < s_size;) {
                const auto s_count = std::min<std::size_t>(7, s_size - s_offset);
                std::size_t s_accepted{};
                if (s_self.m_host.feedBytes(s_data + s_offset, s_count, s_accepted) != FciEndpointStatus::kOk || s_accepted != s_count) return WL_SINK_FAILED;
                s_offset += s_count;
            }
            return WL_SINK_SENT;
        }, this) == WL_OK, "peer sink");
    }
    Peer() {
        s_initLink();
        require(m_host.initialize() == FciEndpointStatus::kOk, "host init");
        m_transport.m_sink = [](void* s_context, wl_io_token_t, const std::uint8_t* s_data, std::size_t s_size) -> wl_sink_result_t {
            auto& s_self = *static_cast<Peer*>(s_context);
            std::lock_guard s_lock(s_self.m_link_reset_mutex);
            std::size_t s_accepted{};
            return wl_feed_bytes(&s_self.m_link, s_data, s_size, &s_accepted) == WL_OK && s_accepted == s_size ? WL_SINK_SENT : WL_SINK_FAILED;
        };
        m_transport.m_context = this;
        require(m_host.attachDirectTransport(m_transport) == FciEndpointStatus::kOk, "host direct transport");
        wl_bulk_receiver_config_t s_receiver{};
        s_receiver.max_object_length = m_ram.size(); s_receiver.max_chunk_size = 2031;
        s_receiver.write_alignment = 1; s_receiver.idle_timeout_ms = 2000;
        s_receiver.sink = {this,
            [](void* s_context, const wl_bulk_descriptor_t*, std::uint64_t* s_offset) -> wl_bulk_sink_result_t {
                auto& s_self = *static_cast<Peer*>(s_context); s_self.m_written = 0; s_self.m_busy = true; *s_offset = 0; return WL_BULK_SINK_OK;
            },
            [](void* s_context, std::uint32_t, std::uint64_t s_offset, const std::uint8_t* s_data, std::size_t s_size) -> wl_bulk_sink_result_t {
                auto& s_self = *static_cast<Peer*>(s_context);
                if (s_self.m_hold_chunk) return WL_BULK_SINK_BUSY;
                if (s_self.m_busy) { s_self.m_busy = false; return WL_BULK_SINK_BUSY; }
                if (s_offset != s_self.m_written || s_size > s_self.m_ram.size() - s_offset) return WL_BULK_SINK_INVALID;
                std::copy_n(s_data, s_size, s_self.m_ram.data() + s_offset); s_self.m_written += s_size; ++s_self.m_writes; return WL_BULK_SINK_OK;
            },
            [](void* s_context, const wl_bulk_descriptor_t* s_descriptor) -> wl_bulk_sink_result_t {
                auto& s_self = *static_cast<Peer*>(s_context);
                if (wl_crc32c(s_self.m_ram.data(), s_self.m_written) != s_descriptor->object_crc32c) return WL_BULK_SINK_INTEGRITY_FAILED;
                ++s_self.m_finishes; return WL_BULK_SINK_OK;
            }, [](void*, std::uint32_t, std::int32_t) {}};
        require(wl_bulk_receiver_init(&m_receiver, &s_receiver) == WL_BULK_OK, "receiver init");
        require(m_host.start() == FciEndpointStatus::kOk, "host start");
        m_thread = std::thread([this] { s_run(); });
    }
    ~Peer() { stop(); }
    void stop() { m_host.stop(); m_stop = true; if (m_thread.joinable()) m_thread.join(); }

    template <typename T, typename Encode>
    void s_response(std::uint16_t s_id, const T& s_value, Encode s_encode) {
        Response s_reply{s_id, std::vector<std::uint8_t>(512)};
        std::size_t s_size{};
        require(s_encode(&s_value, s_reply.m_bytes.data(), s_reply.m_bytes.size(), &s_size) == WL_CODEC_OK, "encode response");
        s_reply.m_bytes.resize(s_size); m_responses.push_back(std::move(s_reply));
    }
    void s_event(const wl_event_t& s_event) {
        if (s_event.type != WL_EVT_RELIABLE_RX && s_event.type != WL_EVT_UNRELIABLE_RX) {
            if (s_event.handle && s_event.handle == m_reboot_handle && s_event.type == WL_EVT_TX_SUCCESS) {
                ++m_reboot_acks;
                m_reset_requested = m_depart_on_reboot;
                m_reboot_handle = 0;
            }
            if (s_event.handle) { wl_tx_result_t s_result{}; (void)wl_tx_take(&m_link, s_event.handle, &s_result); } return;
        }
        switch (s_event.message_id) {
        case GET_DEVICE_INFO_REQUEST_MESSAGE_ID: {
            if (m_silent_info) break;
            test_rpc::get_device_info_request_t s_request{};
            require(test_rpc::get_device_info_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "decode info");
            test_rpc::get_device_info_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_info = true;
            s_reply.operation_id = s_request.operation_id; s_reply.session = s_request.session;
            auto& s_info = s_reply.info;
            s_info.has_protocol_version = s_info.has_firmware_version = true;
            s_info.protocol_version = {.has_major = true, .major = m_session > 199 && m_wrong_protocol ? 9U : 0U,
                .has_minor = true, .minor = 0, .has_patch = true, .patch = 1};
            s_info.firmware_version = {.has_major = true, .major = 0, .has_minor = true, .minor = 0,
                .has_patch = true, .patch = m_version};
            s_info.has_board_name = s_info.has_custom_name = s_info.has_serial = s_info.has_firmware_type = true;
            s_info.board_name = {"H7", 2}; s_info.custom_name = {"Willow", 6};
            s_info.serial = {m_session > 199 && m_wrong_identity ? "OTHER" : "BOARD", 5};
            s_info.firmware_type = FIRMWARE_STANDARD_ARM;
            s_response(GET_DEVICE_INFO_RESPONSE_MESSAGE_ID, s_reply, test_rpc::get_device_info_response_encode); break;
        }
        case GET_BOOT_STATUS_REQUEST_MESSAGE_ID: {
            get_boot_status_request_t s_request{};
            require(get_boot_status_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "decode boot");
            get_boot_status_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_info = true;
            s_reply.operation_id = s_request.operation_id;
            auto& s_info = s_reply.info;
            s_info.has_protocol_version = s_info.has_boot_state = s_info.has_recovery_reason = true;
            s_info.has_active_slot = s_info.has_confirmed_slot = s_info.has_pending_slot = true;
            s_info.has_write_align = s_info.has_max_chunk_size = true;
            s_info.has_slot0_address = s_info.has_slot0_size = s_info.has_slot1_address = s_info.has_slot1_size = s_info.has_bytes_written = true;
            s_info.protocol_version = 1; s_info.write_align = 1; s_info.max_chunk_size = 2031; s_info.bytes_written = m_written;
            s_info.active_slot = SLOT_PRIMARY; s_info.confirmed_slot = SLOT_PRIMARY;
            if (!m_confirm) { s_info.confirmed_slot = SLOT_NONE; s_info.boot_state = BOOT_BOOTING; }
            s_info.slot0_address = 0x08020000; s_info.slot1_address = 0x08080000;
            s_info.slot0_size = s_info.slot1_size = 393216;
            s_response(GET_BOOT_STATUS_RESPONSE_MESSAGE_ID, s_reply, get_boot_status_response_encode); break;
        }
        case START_UPGRADE_REQUEST_MESSAGE_ID: {
            if (m_silent_start) break;
            start_upgrade_request_t s_request{};
            require(start_upgrade_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "decode start");
            (void)wl_bulk_receiver_reset(&m_receiver); m_crc = s_request.image_crc32c;
            start_upgrade_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_transfer_id = true;
            s_reply.has_write_align = s_reply.has_max_chunk_size = s_reply.has_next_offset = true;
            s_reply.operation_id = s_request.operation_id; s_reply.transfer_id = ++m_transfer;
            s_reply.write_align = 1; s_reply.max_chunk_size = 2031;
            s_response(START_UPGRADE_RESPONSE_MESSAGE_ID, s_reply, start_upgrade_response_encode); break;
        }
        case REBOOT_REQUEST_MESSAGE_ID: {
            ++m_reboot_requests;
            if (m_reset_without_reply) { m_reset_requested = true; break; }
            if (m_silent_reboot) break;
            reboot_request_t s_request{};
            require(reboot_request_decode(s_event.payload, s_event.payload_len, &s_request) == WL_CODEC_OK, "decode reboot");
            reboot_response_t s_reply{};
            s_reply.has_operation_id = s_reply.has_status = s_reply.has_mode = true;
            s_reply.operation_id = s_request.operation_id; s_reply.mode = s_request.mode;
            s_reply.status = m_reboot_status.load();
            s_response(REBOOT_RESPONSE_MESSAGE_ID, s_reply, reboot_response_encode); break;
        }
        case BULK_BEGIN_MESSAGE_ID: {
            bulk_begin_t s_message{}; require(bulk_begin_decode(s_event.payload, s_event.payload_len, &s_message) == WL_CODEC_OK, "decode begin");
            wl_bulk_descriptor_t s_descriptor{s_message.transfer_id, s_message.total_length, s_message.requested_chunk_size, s_message.object_crc32c};
            (void)wl_bulk_receiver_on_begin(&m_receiver, &s_descriptor, s_now()); break;
        }
        case BULK_CHUNK_MESSAGE_ID: {
            bulk_chunk_t s_message{}; require(bulk_chunk_decode(s_event.payload, s_event.payload_len, &s_message) == WL_CODEC_OK, "decode chunk");
            wl_bulk_chunk_t s_chunk{s_message.transfer_id, s_message.offset, s_message.data.data, s_message.data.length};
            (void)wl_bulk_receiver_on_chunk(&m_receiver, &s_chunk, s_now()); break;
        }
        case BULK_END_MESSAGE_ID: {
            bulk_end_t s_message{}; require(bulk_end_decode(s_event.payload, s_event.payload_len, &s_message) == WL_CODEC_OK, "decode end");
            (void)wl_bulk_receiver_on_end(&m_receiver, s_message.transfer_id, s_message.total_length, s_message.object_crc32c, s_now()); break;
        }
        case BULK_ABORT_MESSAGE_ID: {
            bulk_abort_t s_message{}; require(bulk_abort_decode(s_event.payload, s_event.payload_len, &s_message) == WL_CODEC_OK, "decode abort");
            (void)wl_bulk_receiver_on_abort(&m_receiver, s_message.transfer_id, s_message.reason, s_now()); break;
        }
        }
        wl_event_release(&m_link, &s_event);
    }
    void s_run() {
        while (!m_stop) {
            wl_event_t s_item{};
            while (wl_poll(&m_link, s_now(), &s_item) == WL_OK) s_event(s_item);
            if (m_reset_requested) {
                m_reset_requested = false; ++m_session; m_version = m_next_version;
                m_responses.clear(); s_initLink();
                // A frame from the restarted peer makes its new session visible,
                // just as telemetry does after boot. This is not a reboot proof
                // unless identity/version/boot queries also succeed.
                get_boot_status_response_t s_announce{};
                s_announce.has_operation_id = s_announce.has_status = true;
                s_response(GET_BOOT_STATUS_RESPONSE_MESSAGE_ID, s_announce, get_boot_status_response_encode);
            }
            if (!m_responses.empty()) {
                const auto& s_response = m_responses.front(); wl_tx_handle_t s_handle{};
                if (wl_send_reliable(&m_link, s_response.m_id, s_response.m_bytes.data(), s_response.m_bytes.size(), s_now(), &s_handle) == WL_OK) {
                    if (s_response.m_id == REBOOT_RESPONSE_MESSAGE_ID && !m_reboot_status)
                        m_reboot_handle = s_handle;
                    m_responses.pop_front();
                }
            }
            (void)wl_bulk_receiver_poll(&m_receiver, s_now());
            wl_bulk_receiver_status_view_t s_view{};
            if (wl_bulk_receiver_status_acquire(&m_receiver, &s_view) == WL_BULK_OK) {
                bulk_status_t s_message{};
                s_message.has_transfer_id = s_message.has_phase = s_message.has_code = s_message.has_next_offset = s_message.has_accepted_chunk_size = true;
                s_message.transfer_id = s_view.status.transfer_id; s_message.phase = s_view.status.phase;
                s_message.code = s_view.status.code; s_message.next_offset = s_view.status.next_offset;
                s_message.accepted_chunk_size = s_view.status.accepted_chunk_size;
                if (m_drop_status && s_message.phase == UPGRADE_BULK_PHASE_CHUNK && s_message.code == UPGRADE_BULK_STATUS_OK) {
                    m_drop_status = false; (void)wl_bulk_receiver_status_release(&m_receiver, &s_view);
                } else if (fci_device_bulk_status_send(&m_link, &s_message, WL_DELIVERY_UNRELIABLE, s_now()).domain == FCI_DEVICE_SEND_OK)
                    (void)wl_bulk_receiver_status_release(&m_receiver, &s_view);
                else (void)wl_bulk_receiver_status_defer(&m_receiver, &s_view);
            }
            std::this_thread::sleep_for(1ms);
        }
    }
};
FirmwareUploadProgress wait(Peer& s_peer) {
    const auto s_deadline = std::chrono::steady_clock::now() + 5s;
    while (s_peer.m_host.upgrade().progress().active() && std::chrono::steady_clock::now() < s_deadline)
        std::this_thread::sleep_for(2ms);
    const auto s_progress = s_peer.m_host.upgrade().progress();
    require(!s_progress.active(), "upload did not terminate"); return s_progress;
}

FirmwareInstallOptions installOptions() {
    return {.m_query_timeout = 100ms, .m_reboot_timeout = 250ms,
            .m_boot_timeout = 700ms, .m_poll_interval = 5ms};
}

void testInstallation() {
    {
        Peer s_peer;
        auto& s_session = s_peer.m_host.firmwareSession();
        const auto s_info = s_session.deviceInfo(1s);
        require(s_info && s_info.m_info->m_serial_number == "BOARD" &&
                s_info.m_info->m_firmware_version.m_patch == 1 && s_info.m_peer_session == 199,
                "same-session device info");
        const auto s_equal = s_session.rebootAndWait({0, 0, 1}, installOptions());
        require(s_equal.m_error == FirmwareUpdateError::kInvalidArgument && !s_peer.m_reboot_requests,
                "equal-version verification must fail before reset");
        auto s_invalid_options = installOptions(); s_invalid_options.m_poll_interval = 0ms;
        require(s_session.rebootAndWait({0, 0, 2}, s_invalid_options).m_error == FirmwareUpdateError::kInvalidArgument,
                "invalid verification timeout admitted");
        const auto s_result = s_session.rebootAndWait({0, 0, 2}, installOptions());
        require(s_result && s_result.m_state == FirmwareInstallState::kTargetConfirmed &&
                s_result.m_reboot.m_request_accepted && s_result.m_reboot.m_departure_observed &&
                s_peer.m_reboot_requests == 1, "new peer target confirmation");
        require(!s_peer.m_host.upgrade().active(), "verification leaked admission gate");
    }
    {
        Peer s_peer; s_peer.m_next_version = 1;
        const auto s_result = s_peer.m_host.firmwareSession().rebootAndWait({0, 0, 2}, installOptions());
        require(!s_result && s_result.m_error == FirmwareUpdateError::kNone &&
                s_result.m_state == FirmwareInstallState::kPreviousFirmware, "old firmware must not count as success");
    }
    {
        Peer s_peer; s_peer.m_depart_on_reboot = false;
        const auto s_result = s_peer.m_host.firmwareSession().rebootAndWait({0, 0, 2}, installOptions());
        require(s_result.m_state == FirmwareInstallState::kUnknown && s_result.m_error == FirmwareUpdateError::kTimeout &&
                s_result.m_reboot.m_request_accepted && !s_result.m_reboot.m_departure_observed &&
                s_peer.m_reboot_requests == 1, "old live session falsely reported reboot or retried reboot");
    }
    {
        Peer s_peer; s_peer.m_confirm = false;
        FirmwareInstallResult s_result{};
        std::thread s_waiter([&] { s_result = s_peer.m_host.firmwareSession().rebootAndWait({0, 0, 2}, installOptions()); });
        const auto s_until = std::chrono::steady_clock::now() + 1s;
        while (!s_peer.m_reboot_requests && std::chrono::steady_clock::now() < s_until) std::this_thread::sleep_for(1ms);
        const auto s_blocked = s_peer.m_host.upgrade().startUpload(std::array<std::uint8_t, 1>{1}, {});
        s_waiter.join();
        require(s_blocked == FirmwareUpdateError::kBusy, "verification admitted a second upload");
        require(!s_result && s_result.m_error == FirmwareUpdateError::kTimeout &&
                s_result.m_device_info && s_result.m_device_info->m_firmware_version.m_patch == 2 &&
                !s_peer.m_host.upgrade().active(), "unconfirmed target was accepted or gate leaked");
    }
    {
        Peer s_peer; s_peer.m_wrong_identity = true;
        const auto s_result = s_peer.m_host.firmwareSession().rebootAndWait({0, 0, 2}, installOptions());
        require(s_result.m_error == FirmwareUpdateError::kIdentityMismatch && !s_result,
                "wrong device was accepted after reconnect");
    }
    {
        Peer s_peer; s_peer.m_wrong_protocol = true;
        const auto s_result = s_peer.m_host.firmwareSession().rebootAndWait({0, 0, 2}, installOptions());
        require(s_result.m_error == FirmwareUpdateError::kProtocolMismatch && !s_result,
                "incompatible firmware protocol accepted");
    }
    {
        Peer s_peer; s_peer.m_reset_without_reply = true;
        const auto s_result = s_peer.m_host.firmwareSession().rebootAndWait({0, 0, 2}, installOptions());
        require(s_result && !s_result.m_reboot.m_request_accepted && s_peer.m_reboot_requests == 1,
                "lost reboot response should be observed, not blindly retried");
    }
    {
        Peer s_peer; s_peer.m_reboot_status = UPGRADE_INVALID_STATE;
        const auto s_result = s_peer.m_host.firmwareSession().rebootAndWait({0, 0, 2}, installOptions());
        require(s_result.m_error == FirmwareUpdateError::kDeviceRejected && !s_result.m_reboot.m_request_accepted &&
                s_result.m_reboot.m_device_status == UPGRADE_INVALID_STATE, "rejected test reboot lost domain status");
    }
    {
        Peer s_peer; s_peer.m_depart_on_reboot = false;
        FirmwareRebootResult s_result{};
        std::thread s_waiter([&] { s_result = s_peer.m_host.upgrade().reboot(FirmwareRebootMode::kNormal, 1s); });
        const auto s_until = std::chrono::steady_clock::now() + 500ms;
        while (!s_peer.m_reboot_requests && std::chrono::steady_clock::now() < s_until) std::this_thread::sleep_for(1ms);
        std::this_thread::sleep_for(30ms);
        s_peer.stop(); s_waiter.join();
        require(s_result.m_error == FirmwareUpdateError::kTransportError && !s_result.m_departure_observed,
                "local close counted as device reboot");
    }
    {
        Peer s_peer; s_peer.m_depart_on_reboot = false;
        FirmwareRebootResult s_result{};
        std::thread s_waiter([&] { s_result = s_peer.m_host.upgrade().reboot(FirmwareRebootMode::kNormal, 1s); });
        const auto s_until = std::chrono::steady_clock::now() + 500ms;
        while (!s_peer.m_reboot_acks && std::chrono::steady_clock::now() < s_until) std::this_thread::sleep_for(1ms);
        s_peer.m_transport.m_state = TransportConnectionState::kReconnecting;
        s_peer.m_host.notify(); s_waiter.join();
        require(s_result.m_error == FirmwareUpdateError::kNone && s_result.m_request_accepted &&
                s_result.m_departure_observed, "USB Reconnecting edge was not recognized as departure");
    }
    {
        Peer s_peer; s_peer.m_hold_chunk = true;
        require(s_peer.m_host.upgrade().startUpload(std::array<std::uint8_t, 1>{1}, {}) == FirmwareUpdateError::kNone,
                "disconnect fixture upload");
        const auto s_until = std::chrono::steady_clock::now() + 1s;
        while (!s_peer.m_host.upgrade().progress().m_busy_responses && std::chrono::steady_clock::now() < s_until)
            std::this_thread::sleep_for(1ms);
        s_peer.m_transport.m_state = TransportConnectionState::kReconnecting;
        s_peer.m_host.notify();
        require(wait(s_peer).m_error == FirmwareUpdateError::kTransportError, "reconnecting upload not failed");
        s_peer.m_transport.m_state = TransportConnectionState::kConnected;
        s_peer.m_host.notify(); std::this_thread::sleep_for(20ms);
        require(s_peer.m_host.upgrade().progress().m_state == FirmwareUploadState::kFailed,
                "reconnected transport replayed failed upload");
    }
    {
        Peer s_peer;
        auto& s_session = s_peer.m_host.firmwareSession();
        require(s_session.deviceInfo(0ms).m_error == FirmwareUpdateError::kInvalidArgument, "zero info timeout admitted");
        s_peer.m_silent_info = true;
        for (int s_i = 0; s_i < 20; ++s_i) {
            const auto s_info = s_session.deviceInfo(15ms);
            require(!s_info, "silent info query succeeded");
            std::this_thread::sleep_for(3ms);
        }
        s_peer.m_silent_info = false;
        FirmwareDeviceInfoResult s_info{};
        const auto s_until = std::chrono::steady_clock::now() + 1s;
        do { s_info = s_session.deviceInfo(100ms); std::this_thread::sleep_for(2ms); }
        while (!s_info && std::chrono::steady_clock::now() < s_until);
        require(static_cast<bool>(s_info), "repeated info timeout leaked operation slots");
        s_peer.stop();
        require(s_session.deviceInfo(50ms).m_error == FirmwareUpdateError::kNotConnected, "info query on closed endpoint");
    }
    require(FirmwareUpdater::connect(DeviceDescriptor{}).m_error == FirmwareConnectionError::kInvalidSelector,
            "wildcard descriptor admitted");
    require(FirmwareUpdater::connect(DeviceSelector::bySerial("")).m_error == FirmwareConnectionError::kInvalidSelector,
            "empty serial admitted");
    require(FirmwareUpdater::connect(DeviceSelector{}, 0ms).m_error == FirmwareConnectionError::kInvalidSelector,
            "invalid connect timeout admitted");
}
}

int main() {
    try {
        std::vector<std::uint8_t> s_image(32768);
        for (std::size_t s_i = 0; s_i < s_image.size(); ++s_i) s_image[s_i] = static_cast<std::uint8_t>(s_i * 41);
        Peer s_peer;
        auto& s_client = s_peer.m_host.upgrade();
        require(s_client.bootStatus(1s).m_max_chunk_size == 2031, "recovery boot query without arm lease");
        const auto s_boot = s_client.bootStatus(1s);
        require(s_boot.m_active_slot == SLOT_PRIMARY && s_boot.m_slot1_address == 0x08080000 &&
                s_boot.m_slot1_size == 393216, "boot slot metadata discarded");
        FirmwareUploadOptions s_fast{}; s_fast.m_status_timeout_ms = 50;
        require(s_client.startUpload(s_image, s_fast) == FirmwareUpdateError::kNone, "start upload");
        require(s_client.startUpload(s_image, {}) == FirmwareUpdateError::kBusy, "second upload admitted");
        require(s_client.reboot(FirmwareRebootMode::kTryBoot, 1s).m_error == FirmwareUpdateError::kBusy,
                "reboot admitted during upload");
        require(s_client.bootStatus(1s).m_error == FirmwareUpdateError::kNone, "management lost during upload");
        const auto s_done = wait(s_peer);
        require(s_done.m_state == FirmwareUploadState::kCompleted && s_done.m_acknowledged_bytes == s_image.size(), "upload failed");
        require(s_done.m_busy_responses > 0 && s_done.m_retries > 0, "BUSY/status-loss path not exercised");
        s_peer.m_hold_chunk = true;
        require(s_client.startUpload(s_image, {}) == FirmwareUpdateError::kNone, "restart upload");
        const auto s_abort_deadline = std::chrono::steady_clock::now() + 2s;
        while (s_client.progress().m_busy_responses == 0 && std::chrono::steady_clock::now() < s_abort_deadline)
            std::this_thread::sleep_for(1ms);
        require(s_client.progress().m_busy_responses != 0, "cancel fixture never reached a blocked write");
        s_client.cancel();
        require(wait(s_peer).m_state == FirmwareUploadState::kCancelled, "abort did not terminate");
        s_peer.m_hold_chunk = false;
        s_peer.m_silent_start = true;
        FirmwareUploadOptions s_options{}; s_options.m_timeout = 150ms;
        require(s_client.startUpload(s_image, s_options) == FirmwareUpdateError::kNone, "timeout upload");
        require(wait(s_peer).m_error == FirmwareUpdateError::kTimeout, "StartUpgrade timeout");
        require(s_client.reboot(FirmwareRebootMode::kTryBoot, 1s).m_error == FirmwareUpdateError::kNone, "reboot RPC");
        s_peer.m_reboot_status = UPGRADE_INVALID_STATE;
        const auto s_rejected = s_client.reboot(FirmwareRebootMode::kTryBoot, 1s);
        require(s_rejected.m_error == FirmwareUpdateError::kDeviceRejected &&
                s_rejected.m_device_status == UPGRADE_INVALID_STATE, "reboot rejection lost");
        require(s_client.reboot(static_cast<FirmwareRebootMode>(1), 1s).m_error == FirmwareUpdateError::kInvalidArgument,
                "unsupported persistent recovery mode admitted");
        s_peer.m_silent_reboot = true;
        FirmwareRebootResult s_timed_out{};
        std::thread s_reboot([&] { s_timed_out = s_client.reboot(FirmwareRebootMode::kNormal, 150ms); });
        const auto s_queued_until = std::chrono::steady_clock::now() + 100ms;
        while (!s_client.active() && std::chrono::steady_clock::now() < s_queued_until) std::this_thread::sleep_for(1ms);
        const auto s_blocked_upload = s_client.startUpload(s_image, {});
        s_reboot.join();
        require(s_blocked_upload == FirmwareUpdateError::kBusy, "upload admitted during pending reboot");
        require(s_timed_out.m_error == FirmwareUpdateError::kTimeout, "silent reboot did not time out");
        const auto s_cleanup_until = std::chrono::steady_clock::now() + 1s;
        while (s_client.active() && std::chrono::steady_clock::now() < s_cleanup_until) std::this_thread::sleep_for(1ms);
        require(!s_client.active(), "reboot deadline leaked admission gate");
        require(s_client.startUpload(s_image, {}) == FirmwareUpdateError::kNone, "shutdown upload");
        s_peer.stop();
        require(!s_client.progress().active(), "shutdown leaked active upload");
        require(s_peer.m_finishes == 1 && std::equal(s_image.begin(), s_image.end(), s_peer.m_ram.begin()), "stored bytes/commit mismatch");
        require(s_client.bootStatus(50ms).m_error == FirmwareUpdateError::kNotConnected, "closed endpoint admitted boot RPC");
        require(s_client.reboot(FirmwareRebootMode::kNormal, 50ms).m_error == FirmwareUpdateError::kNotConnected,
                "closed endpoint admitted reboot RPC");
        testInstallation();
        std::puts("PASS: upgrade, same-session identity, reboot ACK/departure, target/old/unknown boot, timeouts, admission, close");
    } catch (const std::exception& s_error) { std::fprintf(stderr, "FAIL: %s\n", s_error.what()); return 1; }
}
