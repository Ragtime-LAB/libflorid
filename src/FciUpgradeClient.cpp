#include "florid/detail/FciUpgradeClient.hpp"

#include <wirelink/crc.h>
#include <algorithm>

namespace florid::detail {
namespace {
std::uint32_t s_elapsed(std::uint32_t s_now, std::uint32_t s_since) {
    // A producer can enqueue after the owner sampled this pass's clock.
    // Treat that small future age as zero, not a 2^32 ms timeout.
    const auto s_delta = static_cast<std::int32_t>(s_now - s_since);
    return s_delta < 0 ? 0U : static_cast<std::uint32_t>(s_delta);
}
bool s_retryable(const fci_device_runtime_result_t& s_result) {
    return (s_result.domain == FCI_DEVICE_RUNTIME_CORE_ERROR &&
        (s_result.detail.rpc.core_result == WL_ERR_BUSY || s_result.detail.rpc.core_result == WL_ERR_WOULD_BLOCK)) ||
        (s_result.domain == FCI_DEVICE_RUNTIME_RPC_ERROR && s_result.detail.rpc.rpc_result == WL_RPC_ERR_NO_SLOT);
}
}

std::uint32_t FciUpgradeClient::s_now() {
    return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void FciUpgradeClient::configure(fci_device_endpoint_config_t& s_config) {
    s_config.on_bulk_status = s_status;
    s_config.bulk_status_user_data = this;
}

void FciUpgradeClient::attach(fci_device_endpoint_t& s_endpoint, void (*s_notify)(void*), void* s_context) {
    m_endpoint = &s_endpoint; m_notify = s_notify; m_notify_context = s_context;
    m_service = {
        .user_data = this,
        .progress = [](void* s_self, wl_ctx_t*, wl_time_ms_t s_now_ms) -> std::uint8_t {
            auto& s_client = *static_cast<FciUpgradeClient*>(s_self);
            std::lock_guard s_lock(s_client.m_mutex);
            s_client.s_progress(s_now_ms);
            return 0;
        },
        .deadline_hint = [](const void* s_self, wl_time_ms_t) -> std::uint32_t {
            const auto& s_client = *static_cast<const FciUpgradeClient*>(s_self);
            std::lock_guard s_lock(s_client.m_mutex);
            return s_client.m_progress.active() || s_client.m_boot_pending || s_client.m_reboot_pending ? 10U : UINT32_MAX;
        },
        .on_peer_session = [](void* s_self, wl_ctx_t*, std::uint64_t s_previous,
                              std::uint64_t s_current, wl_time_ms_t) {
            auto& s_client = *static_cast<FciUpgradeClient*>(s_self);
            { std::lock_guard s_lock(s_client.m_mutex); s_client.m_peer_session = s_current; }
            if (s_previous) s_client.disconnected(true);
        },
        .on_close = [](void* s_self, wl_ctx_t*) { static_cast<FciUpgradeClient*>(s_self)->disconnected(); },
    };
}

void FciUpgradeClient::connected(bool s_running) {
    std::lock_guard s_lock(m_mutex);
    m_running = s_running;
}

void FciUpgradeClient::s_cancelRpc(std::uint32_t& s_id) {
    if (!s_id) return;
    wl_rpc_client_result_t s_result{};
    if (wl_rpc_client_get(s_runtime()->rpc_client, s_id, &s_result) == WL_RPC_OK) {
        (void)wl_rpc_client_cancel(s_runtime()->rpc_client, s_id);
        if (s_result.tx_handle) (void)wl_tx_cancel(wl_endpoint_link(fci_device_endpoint_handle(m_endpoint)), s_result.tx_handle);
        (void)wl_rpc_client_release(s_runtime()->rpc_client, s_id);
    }
    s_id = 0;
}

void FciUpgradeClient::transportState(bool s_connected) {
    // Called only by the endpoint owner. Repeated offline service passes must
    // not cancel the read-only RPCs used to discover a reconnected peer.
    const bool s_departed = m_transport_connected && !s_connected;
    m_transport_connected = s_connected;
    if (s_departed) disconnected(true);
}

FirmwareUpdateError FciUpgradeClient::beginVerification() {
    std::lock_guard s_lock(m_mutex);
    if (!m_running) return FirmwareUpdateError::kNotConnected;
    if (m_progress.active() || m_reboot_pending || m_verifying) return FirmwareUpdateError::kBusy;
    m_verifying = true;
    m_notify(m_notify_context);
    return FirmwareUpdateError::kNone;
}

void FciUpgradeClient::endVerification() {
    std::lock_guard s_lock(m_mutex);
    m_verifying = false;
    m_notify(m_notify_context);
}

void FciUpgradeClient::disconnected(bool s_departed) {
    std::lock_guard s_lock(m_mutex);
    // The response can already be decoded by the RPC runtime but not yet
    // collected by this service when transport loss is observed.
    if (m_reboot_pending && m_reboot_id) s_pollReboot(s_now());
    s_cancelRpc(m_boot_id); s_cancelRpc(m_start_id); s_cancelRpc(m_reboot_id);
    if (m_progress.active()) s_finish(FirmwareUpdateError::kTransportError);
    if (m_boot_pending) {
        m_boot = {.m_error = FirmwareUpdateError::kTransportError};
        m_boot_done = true; m_boot_pending = false; m_changed.notify_all();
    }
    if (m_reboot_pending) {
        m_reboot.m_departure_observed = s_departed;
        m_reboot.m_error = s_departed && m_reboot.m_request_accepted
            ? FirmwareUpdateError::kNone : FirmwareUpdateError::kTransportError;
        m_reboot_done = true; m_reboot_pending = false; m_changed.notify_all();
    }
}

BootStatus FciUpgradeClient::bootStatus(std::chrono::milliseconds s_timeout) {
    std::unique_lock s_call_lock(m_boot_call_mutex, std::try_to_lock);
    if (!s_call_lock.owns_lock()) return {.m_error = FirmwareUpdateError::kBusy};
    if (s_timeout.count() <= 0 || s_timeout.count() >= 0x80000000LL)
        return {.m_error = FirmwareUpdateError::kInvalidArgument};
    std::unique_lock s_lock(m_mutex);
    if (!m_running) return {.m_error = FirmwareUpdateError::kNotConnected};
    if (m_boot_pending) return {.m_error = FirmwareUpdateError::kBusy};
    m_boot_pending = true; m_boot_done = false;
    m_boot_since_ms = s_now(); m_boot_timeout_ms = static_cast<std::uint32_t>(s_timeout.count());
    m_notify(m_notify_context);
    if (!m_changed.wait_for(s_lock, s_timeout, [this] { return m_boot_done; }))
        return {.m_error = FirmwareUpdateError::kTimeout};
    return m_boot;
}

FirmwareUpdateError FciUpgradeClient::startUpload(std::span<const std::uint8_t> s_image,
                                                const FirmwareUploadOptions& s_options) {
    if (s_image.empty() || s_options.m_timeout.count() <= 0 || s_options.m_timeout.count() >= 0x80000000LL ||
        !s_options.m_status_timeout_ms || s_options.m_status_timeout_ms >= 0x80000000U ||
        !s_options.m_busy_retry_ms || s_options.m_busy_retry_ms >= 0x80000000U)
        return FirmwareUpdateError::kInvalidArgument;
    std::lock_guard s_lock(m_mutex);
    if (!m_running) return FirmwareUpdateError::kNotConnected;
    if (m_progress.active() || m_reboot_pending || m_verifying) return FirmwareUpdateError::kBusy;
    m_image.assign(s_image.begin(), s_image.end());
    m_crc = wl_crc32c(m_image.data(), m_image.size());
    m_options = s_options; m_since_ms = s_now(); m_cancel = false;
    m_progress = {.m_state = FirmwareUploadState::kQueued, .m_total_bytes = m_image.size()};
    m_notify(m_notify_context);
    return FirmwareUpdateError::kNone;
}

FirmwareRebootResult FciUpgradeClient::reboot(FirmwareRebootMode s_mode, std::chrono::milliseconds s_timeout,
                                            bool s_verification) {
    std::unique_lock s_call_lock(m_reboot_call_mutex, std::try_to_lock);
    if (!s_call_lock.owns_lock()) return {.m_error = FirmwareUpdateError::kBusy};
    if ((s_mode != FirmwareRebootMode::kNormal && s_mode != FirmwareRebootMode::kTryBoot) ||
        s_timeout.count() <= 0 || s_timeout.count() >= 0x80000000LL)
        return {.m_error = FirmwareUpdateError::kInvalidArgument};
    std::unique_lock s_lock(m_mutex);
    if (!m_running) return {.m_error = FirmwareUpdateError::kNotConnected};
    if (m_progress.active() || m_reboot_pending || (m_verifying && !s_verification))
        return {.m_error = FirmwareUpdateError::kBusy};
    m_reboot = {};
    m_reboot_pending = true; m_reboot_done = false; m_reboot_mode = s_mode;
    m_reboot_since_ms = s_now(); m_reboot_timeout_ms = static_cast<std::uint32_t>(s_timeout.count());
    m_notify(m_notify_context);
    if (!m_changed.wait_for(s_lock, s_timeout, [this] { return m_reboot_done; })) {
        auto s_result = m_reboot;
        s_result.m_error = FirmwareUpdateError::kTimeout;
        return s_result;
    }
    return m_reboot;
}

FirmwareUploadProgress FciUpgradeClient::progress() const {
    std::lock_guard s_lock(m_mutex);
    return m_progress;
}

void FciUpgradeClient::cancel() {
    std::lock_guard s_lock(m_mutex);
    if (m_progress.active()) { m_cancel = true; m_notify(m_notify_context); }
}

void FciUpgradeClient::s_finish(FirmwareUpdateError s_error) {
    m_progress.m_error = s_error;
    m_progress.m_state = s_error == FirmwareUpdateError::kNone ? FirmwareUploadState::kCompleted :
        s_error == FirmwareUpdateError::kCancelled ? FirmwareUploadState::kCancelled : FirmwareUploadState::kFailed;
    m_image.clear();
}

void FciUpgradeClient::s_pollBoot(std::uint32_t s_now_ms) {
    if (!m_boot_pending) return;
    const auto s_age = s_elapsed(s_now_ms, m_boot_since_ms);
    if (s_age >= m_boot_timeout_ms) {
        s_cancelRpc(m_boot_id); m_boot = {.m_error = FirmwareUpdateError::kTimeout};
    } else if (!m_boot_id) {
        get_boot_status_request_t s_request{};
        const auto s_result = fci_device_endpoint_get_boot_status_start(m_endpoint, &s_request, m_boot_timeout_ms - s_age);
        if (fci_device_runtime_result_ok(&s_result)) { m_boot_id = s_result.detail.rpc.operation_id; return; }
        if (s_retryable(s_result)) return;
        m_boot = {.m_error = FirmwareUpdateError::kTransportError};
    } else {
        wl_rpc_client_result_t s_result{};
        if (fci_device_endpoint_get_boot_status_inspect(m_endpoint, m_boot_id, &s_result) != WL_RPC_OK) {
            m_boot = {.m_error = FirmwareUpdateError::kProtocolError};
        } else if (s_result.state < WL_RPC_CLIENT_COMPLETED) return;
        else {
            get_boot_status_response_t s_response{};
            const auto s_decoded = fci_device_get_boot_status_client_decode(&s_result, &s_response);
            m_boot = {.m_error = FirmwareUpdateError::kProtocolError, .m_device_status = s_result.application_status};
            if (s_result.state == WL_RPC_CLIENT_TIMED_OUT) m_boot.m_error = FirmwareUpdateError::kTimeout;
            else if (s_result.application_status) m_boot.m_error = FirmwareUpdateError::kDeviceRejected;
            else if (fci_device_runtime_result_ok(&s_decoded) && s_response.has_info) {
                const auto& s_info = s_response.info;
                m_boot = {.m_protocol_version = s_info.protocol_version, .m_boot_state = s_info.boot_state,
                    .m_write_alignment = s_info.write_align, .m_max_chunk_size = s_info.max_chunk_size,
                    .m_bytes_written = s_info.bytes_written,
                    .m_active_transfer_id = s_info.has_active_transfer_id ? s_info.active_transfer_id : 0,
                    .m_recovery_reason = s_info.recovery_reason,
                    .m_active_slot = s_info.active_slot, .m_confirmed_slot = s_info.confirmed_slot,
                    .m_pending_slot = s_info.pending_slot, .m_slot0_address = s_info.slot0_address,
                    .m_slot0_size = s_info.slot0_size, .m_slot1_address = s_info.slot1_address,
                    .m_slot1_size = s_info.slot1_size};
            }
        }
        (void)fci_device_endpoint_get_boot_status_release(m_endpoint, m_boot_id); m_boot_id = 0;
    }
    m_boot_pending = false; m_boot_done = true; m_changed.notify_all();
}

void FciUpgradeClient::s_pollReboot(std::uint32_t s_now_ms) {
    if (!m_reboot_pending) return;
    const auto s_age = s_elapsed(s_now_ms, m_reboot_since_ms);
    if (s_age >= m_reboot_timeout_ms) {
        s_cancelRpc(m_reboot_id); m_reboot.m_error = FirmwareUpdateError::kTimeout;
    } else if (m_reboot.m_request_accepted) {
        // Keep the owner/transport alive to deliver the response ACK. Only
        // observed departure completes this phase; a fixed sleep is not proof.
        return;
    } else if (!m_reboot_id) {
        reboot_request_t s_request{};
        s_request.has_mode = true; s_request.mode = static_cast<reboot_mode_t>(m_reboot_mode);
        const auto s_result = fci_device_endpoint_reboot_start(m_endpoint, &s_request, m_reboot_timeout_ms - s_age);
        if (fci_device_runtime_result_ok(&s_result)) { m_reboot_id = s_result.detail.rpc.operation_id; return; }
        if (s_retryable(s_result)) return;
        m_reboot = {.m_error = FirmwareUpdateError::kTransportError};
    } else {
        wl_rpc_client_result_t s_result{};
        if (fci_device_endpoint_reboot_inspect(m_endpoint, m_reboot_id, &s_result) != WL_RPC_OK) {
            m_reboot = {.m_error = FirmwareUpdateError::kProtocolError};
        } else if (s_result.state < WL_RPC_CLIENT_COMPLETED) return;
        else {
            reboot_response_t s_response{};
            const auto s_decoded = fci_device_reboot_client_decode(&s_result, &s_response);
            m_reboot = {.m_error = FirmwareUpdateError::kProtocolError, .m_device_status = s_result.application_status};
            if (s_result.state == WL_RPC_CLIENT_TIMED_OUT) m_reboot.m_error = FirmwareUpdateError::kTimeout;
            else if (s_result.application_status) m_reboot.m_error = FirmwareUpdateError::kDeviceRejected;
            else if (fci_device_runtime_result_ok(&s_decoded) && s_response.has_mode &&
                     s_response.mode == static_cast<reboot_mode_t>(m_reboot_mode)) {
                m_reboot.m_error = FirmwareUpdateError::kNone;
                m_reboot.m_request_accepted = true;
            }
        }
        (void)fci_device_endpoint_reboot_release(m_endpoint, m_reboot_id); m_reboot_id = 0;
        if (m_reboot.m_request_accepted) return;
    }
    m_reboot_pending = false; m_reboot_done = true; m_changed.notify_all();
}

void FciUpgradeClient::s_pollStart(std::uint32_t s_now_ms) {
    if (!m_start_id) {
        start_upgrade_request_t s_request{};
        s_request.has_image_id = s_request.has_target_slot = s_request.has_flags = true;
        s_request.has_image_version = s_request.has_image_size = s_request.has_image_crc32c = true;
        s_request.image_version.has_major = s_request.image_version.has_minor = true;
        s_request.image_version.has_revision = s_request.image_version.has_build = true;
        s_request.image_id = IMAGE_APPLICATION; s_request.target_slot = SLOT_SECONDARY;
        s_request.image_size = m_image.size(); s_request.image_crc32c = m_crc;
        const auto s_remaining = static_cast<std::uint32_t>(m_options.m_timeout.count()) - s_elapsed(s_now_ms, m_since_ms);
        const auto s_result = fci_device_endpoint_start_upgrade_start(m_endpoint, &s_request, s_remaining);
        if (fci_device_runtime_result_ok(&s_result)) {
            m_start_id = s_result.detail.rpc.operation_id; m_progress.m_state = FirmwareUploadState::kStarting;
        } else if (!s_retryable(s_result)) s_finish(FirmwareUpdateError::kTransportError);
        return;
    }
    wl_rpc_client_result_t s_result{};
    if (fci_device_endpoint_start_upgrade_inspect(m_endpoint, m_start_id, &s_result) != WL_RPC_OK) {
        s_finish(FirmwareUpdateError::kProtocolError); s_cancelRpc(m_start_id); return;
    }
    if (s_result.state < WL_RPC_CLIENT_COMPLETED) return;
    start_upgrade_response_t s_response{};
    const auto s_decoded = fci_device_start_upgrade_client_decode(&s_result, &s_response);
    (void)fci_device_endpoint_start_upgrade_release(m_endpoint, m_start_id); m_start_id = 0;
    m_progress.m_device_status = s_result.application_status;
    if (s_result.application_status) { s_finish(FirmwareUpdateError::kDeviceRejected); return; }
    if (!fci_device_runtime_result_ok(&s_decoded) || !s_response.has_transfer_id || !s_response.transfer_id ||
        !s_response.has_write_align || !s_response.write_align || (s_response.write_align & (s_response.write_align - 1U)) ||
        !s_response.has_max_chunk_size || s_response.max_chunk_size < s_response.write_align ||
        s_response.max_chunk_size > 2031 || s_response.max_chunk_size % s_response.write_align ||
        (s_response.has_next_offset && s_response.next_offset != 0)) {
        s_finish(s_result.state == WL_RPC_CLIENT_TIMED_OUT ? FirmwareUpdateError::kTimeout : FirmwareUpdateError::kProtocolError); return;
    }
    wl_bulk_sender_config_t s_config{m_options.m_status_timeout_ms, m_options.m_busy_retry_ms, m_options.m_max_retries};
    (void)wl_bulk_sender_init(&m_sender, &s_config);
    wl_bulk_descriptor_t s_descriptor{s_response.transfer_id, m_image.size(), s_response.max_chunk_size, m_crc};
    if (wl_bulk_sender_start(&m_sender, &s_descriptor) != WL_BULK_OK) { s_finish(FirmwareUpdateError::kProtocolError); return; }
    m_progress.m_transfer_id = s_response.transfer_id;
    m_progress.m_state = FirmwareUploadState::kUploading;
}

std::int32_t FciUpgradeClient::s_status(void* s_self, const bulk_status_t* s_message, wl_delivery_t) {
    auto& s_client = *static_cast<FciUpgradeClient*>(s_self);
    std::lock_guard s_lock(s_client.m_mutex);
    if (s_client.m_progress.m_state != FirmwareUploadState::kUploading &&
        s_client.m_progress.m_state != FirmwareUploadState::kCancelling) return 0;
    wl_bulk_status_t s_status{s_message->transfer_id, s_message->phase, s_message->code,
        s_message->next_offset, s_message->accepted_chunk_size};
    wl_time_ms_t s_now_ms{};
    (void)wl_endpoint_now(fci_device_endpoint_handle(s_client.m_endpoint), &s_now_ms);
    (void)wl_bulk_sender_on_status(&s_client.m_sender, &s_status, s_now_ms);
    return 0;
}

void FciUpgradeClient::s_progress(std::uint32_t s_now_ms) {
    s_pollBoot(s_now_ms);
    s_pollReboot(s_now_ms);
    if (!m_progress.active()) return;
    if (s_elapsed(s_now_ms, m_since_ms) >= static_cast<std::uint32_t>(m_options.m_timeout.count())) {
        s_cancelRpc(m_start_id); s_finish(FirmwareUpdateError::kTimeout); return;
    }
    if (m_cancel && m_progress.m_state == FirmwareUploadState::kQueued) {
        s_finish(FirmwareUpdateError::kCancelled); return;
    }
    if (m_progress.m_state == FirmwareUploadState::kQueued || m_progress.m_state == FirmwareUploadState::kStarting) {
        // A cancelled Start must still be resolved to learn the transfer ID;
        // then send Abort. Cancelling its RPC would strand the device reservation.
        s_pollStart(s_now_ms); return;
    }
    if (m_cancel && m_progress.m_state != FirmwareUploadState::kCancelling) {
        (void)wl_bulk_sender_request_abort(&m_sender, WL_ERR_CANCELLED);
        m_progress.m_state = FirmwareUploadState::kCancelling;
    }
    (void)wl_bulk_sender_poll(&m_sender, s_now_ms);
    wl_bulk_sender_result_t s_result{};
    (void)wl_bulk_sender_get_result(&m_sender, &s_result);
    wl_bulk_sender_stats_t s_stats{};
    (void)wl_bulk_sender_get_stats(&m_sender, &s_stats);
    m_progress.m_acknowledged_bytes = s_result.next_offset;
    m_progress.m_retries = s_stats.retries; m_progress.m_busy_responses = s_stats.busy_responses;
    if (s_result.state == WL_BULK_SENDER_COMPLETED) { s_finish(FirmwareUpdateError::kNone); return; }
    if (s_result.state == WL_BULK_SENDER_ABORTED) {
        s_finish(m_progress.m_error == FirmwareUpdateError::kNone ? FirmwareUpdateError::kCancelled : m_progress.m_error); return;
    }
    if (s_result.state == WL_BULK_SENDER_FAILED) {
        if (m_progress.m_state == FirmwareUploadState::kCancelling) { s_finish(FirmwareUpdateError::kTransportError); return; }
        m_progress.m_error = FirmwareUpdateError::kProtocolError;
        m_progress.m_device_status = s_result.status;
        (void)wl_bulk_sender_request_abort(&m_sender, WL_ERR_IO);
        m_progress.m_state = FirmwareUploadState::kCancelling;
    }
    wl_bulk_sender_action_t s_action{};
    if (wl_bulk_sender_action_acquire(&m_sender, &s_action) != WL_BULK_OK) return;
    fci_device_send_result_t s_sent{};
    switch (s_action.phase) {
    case WL_BULK_PHASE_BEGIN: {
        bulk_begin_t s_message{};
        s_message.has_transfer_id = s_message.has_total_length = s_message.has_requested_chunk_size = s_message.has_object_crc32c = true;
        s_message.transfer_id = s_action.descriptor.transfer_id; s_message.total_length = s_action.descriptor.total_length;
        s_message.requested_chunk_size = s_action.descriptor.requested_chunk_size; s_message.object_crc32c = s_action.descriptor.object_crc32c;
        s_sent = fci_device_endpoint_send_bulk_begin(m_endpoint, &s_message); break;
    }
    case WL_BULK_PHASE_CHUNK: {
        bulk_chunk_t s_message{};
        s_message.has_transfer_id = s_message.has_offset = s_message.has_data = true;
        s_message.transfer_id = s_action.descriptor.transfer_id; s_message.offset = s_action.offset;
        s_message.data = {m_image.data() + s_action.offset, s_action.length};
        s_sent = fci_device_endpoint_send_bulk_chunk(m_endpoint, &s_message); break;
    }
    case WL_BULK_PHASE_END: {
        bulk_end_t s_message{};
        s_message.has_transfer_id = s_message.has_total_length = s_message.has_object_crc32c = true;
        s_message.transfer_id = s_action.descriptor.transfer_id; s_message.total_length = s_action.descriptor.total_length;
        s_message.object_crc32c = s_action.descriptor.object_crc32c;
        s_sent = fci_device_endpoint_send_bulk_end(m_endpoint, &s_message); break;
    }
    case WL_BULK_PHASE_ABORT: {
        bulk_abort_t s_message{};
        s_message.has_transfer_id = s_message.has_reason = true;
        s_message.transfer_id = s_action.descriptor.transfer_id; s_message.reason = s_action.abort_reason;
        s_sent = fci_device_endpoint_send_bulk_abort(m_endpoint, &s_message); break;
    }
    default: (void)wl_bulk_sender_action_defer(&m_sender, &s_action); s_finish(FirmwareUpdateError::kProtocolError); return;
    }
    if (s_sent.domain == FCI_DEVICE_SEND_OK) (void)wl_bulk_sender_action_submitted(&m_sender, &s_action, s_now_ms);
    else (void)wl_bulk_sender_action_defer(&m_sender, &s_action);
}

} // namespace florid::detail
