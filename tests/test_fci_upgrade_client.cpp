#include "florid/detail/FciWirelinkEndpoint.hpp"
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

// Independent wire peer. Uses the real bulk receiver and raw FCI codecs;
// never feeds two contexts with the same input or drives the host owner.
struct Peer {
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
    std::thread m_thread;
    struct Response { std::uint16_t m_id{}; std::vector<std::uint8_t> m_bytes; };
    std::deque<Response> m_responses;

    static std::uint32_t s_now() {
        return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    Peer() {
        wl_config_t s_config{};
        s_config.max_payload_len = m_payload.size(); s_config.envelope = WL_ENVELOPE_COBS_STREAM;
        s_config.integrity = WL_INTEGRITY_NONE; s_config.session_id = 199;
        s_config.max_retries = 2; s_config.ack_timeout_ms = 20; s_config.max_transmission_unit = m_tx.size();
        wl_storage_t s_storage{m_payload.data(), m_payload.size(), m_tx.data(), m_tx.size(),
            m_control.data(), m_control.size(), m_rx.data(), m_rx.size(), m_fallback.data(), m_fallback.size()};
        require(wl_init(&m_link, &s_config, &s_storage) == WL_OK, "peer init");
        require(m_host.initialize() == FciEndpointStatus::kOk, "host init");
        require(m_host.setSink([](void* s_context, wl_io_token_t, const std::uint8_t* s_data, std::size_t s_size) -> wl_sink_result_t {
            auto& s_self = *static_cast<Peer*>(s_context);
            std::size_t s_accepted{};
            return wl_feed_bytes(&s_self.m_link, s_data, s_size, &s_accepted) == WL_OK && s_accepted == s_size ? WL_SINK_SENT : WL_SINK_FAILED;
        }, this) == FciEndpointStatus::kOk, "host sink");
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
            if (s_event.handle) { wl_tx_result_t s_result{}; (void)wl_tx_take(&m_link, s_event.handle, &s_result); } return;
        }
        switch (s_event.message_id) {
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
            if (!m_responses.empty()) {
                const auto& s_response = m_responses.front(); wl_tx_handle_t s_handle{};
                if (wl_send_reliable(&m_link, s_response.m_id, s_response.m_bytes.data(), s_response.m_bytes.size(), s_now(), &s_handle) == WL_OK) m_responses.pop_front();
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
}

int main() {
    try {
        std::vector<std::uint8_t> s_image(32768);
        for (std::size_t s_i = 0; s_i < s_image.size(); ++s_i) s_image[s_i] = static_cast<std::uint8_t>(s_i * 41);
        Peer s_peer;
        auto& s_client = s_peer.m_host.upgrade();
        require(s_client.bootStatus(1s).m_max_chunk_size == 2031, "recovery boot query without arm lease");
        require(s_client.startUpload(s_image, {}) == FirmwareUpdateError::kNone, "start upload");
        require(s_client.startUpload(s_image, {}) == FirmwareUpdateError::kBusy, "second upload admitted");
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
        require(s_client.startUpload(s_image, {}) == FirmwareUpdateError::kNone, "shutdown upload");
        s_peer.stop();
        require(!s_client.progress().active(), "shutdown leaked active upload");
        require(s_peer.m_finishes == 1 && std::equal(s_image.begin(), s_image.end(), s_peer.m_ram.begin()), "stored bytes/commit mismatch");
        require(s_client.bootStatus(50ms).m_error == FirmwareUpdateError::kNotConnected, "closed endpoint admitted boot RPC");
        std::puts("PASS: composed upgrade client, management, BUSY, lost status, cancel, timeout, close");
    } catch (const std::exception& s_error) { std::fprintf(stderr, "FAIL: %s\n", s_error.what()); return 1; }
}
