#include "florid/detail/FirmwareUpdateSession.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"
#include "florid/DeviceDiscovery.hpp"

#include <algorithm>
#include <thread>

namespace florid::detail {
namespace {
using namespace std::chrono_literals;
bool s_validTimeout(std::chrono::milliseconds s_timeout) {
    return s_timeout.count() > 0 && s_timeout.count() < 0x80000000LL;
}
bool s_sameVersion(const Version& s_a, const Version& s_b) {
    return s_a.m_major == s_b.m_major && s_a.m_minor == s_b.m_minor && s_a.m_patch == s_b.m_patch;
}
FirmwareUpdateError s_error(FciEndpointStatus s_status) {
    switch (s_status) {
    case FciEndpointStatus::kOk: return FirmwareUpdateError::kNone;
    case FciEndpointStatus::kBusy:
    case FciEndpointStatus::kQueueFull: return FirmwareUpdateError::kBusy;
    case FciEndpointStatus::kNotReady: return FirmwareUpdateError::kNotConnected;
    case FciEndpointStatus::kTimeout: return FirmwareUpdateError::kTimeout;
    case FciEndpointStatus::kDomainError: return FirmwareUpdateError::kDeviceRejected;
    case FciEndpointStatus::kCancelled: return FirmwareUpdateError::kCancelled;
    case FciEndpointStatus::kLinkError: return FirmwareUpdateError::kTransportError;
    case FciEndpointStatus::kInvalidArgument: return FirmwareUpdateError::kInvalidArgument;
    default: return FirmwareUpdateError::kProtocolError;
    }
}
}

FirmwareDeviceInfoResult FirmwareUpdateSession::deviceInfo(std::chrono::milliseconds s_timeout) {
    std::unique_lock s_lock(m_call_mutex, std::try_to_lock);
    if (!s_lock.owns_lock()) return {.m_error = FirmwareUpdateError::kBusy};
    return s_deviceInfo(s_timeout);
}

FirmwareDeviceInfoResult FirmwareUpdateSession::s_deviceInfo(std::chrono::milliseconds s_timeout) {
    if (!s_validTimeout(s_timeout)) return {.m_error = FirmwareUpdateError::kInvalidArgument};
    const auto s_deadline = std::chrono::steady_clock::now() + s_timeout;
    FciOperationResult s_operation{};
    DeviceInfo s_info{};
    if (m_info_request) {
        // A caller-side deadline can expire just before the owner publishes its
        // timeout. Don't accumulate orphan RPCs on repeated offline polling.
        if (m_endpoint.takeDeviceInfo(m_info_request, s_operation, s_info) == FciEndpointStatus::kBusy)
            return {.m_error = FirmwareUpdateError::kBusy};
        m_info_request = 0;
    }
    const auto s_peer = m_endpoint.upgrade().peerSession();
    const auto s_submit = m_endpoint.getDeviceInfo(static_cast<std::uint32_t>(s_timeout.count()));
    if (s_submit.m_status != FciEndpointStatus::kOk) return {.m_error = s_error(s_submit.m_status)};
    m_info_request = s_submit.m_request_id;
    (void)m_endpoint.waitOperation(m_info_request, s_timeout, s_operation);
    const auto s_status = m_endpoint.takeDeviceInfo(m_info_request, s_operation, s_info);
    if (s_status == FciEndpointStatus::kBusy) return {.m_error = FirmwareUpdateError::kTimeout};
    m_info_request = 0;
    if (s_status != FciEndpointStatus::kOk)
        return {.m_error = s_error(s_status), .m_device_status = s_operation.m_domain_status};
    const auto s_response_peer = m_endpoint.upgrade().peerSession();
    if (!s_response_peer) return {.m_error = FirmwareUpdateError::kProtocolError};
    if (s_peer && s_peer != s_response_peer)
        return {.m_error = FirmwareUpdateError::kTransportError};
    if (!s_peer) {
        // The very first response establishes a peer identity. Re-read with
        // that identity known BEFORE submission, within the original deadline;
        // otherwise a restart between completion and take could tag old info
        // with the new session. Subsequent calls need only one RPC.
        const auto s_remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            s_deadline - std::chrono::steady_clock::now());
        if (s_remaining.count() <= 0) return {.m_error = FirmwareUpdateError::kTimeout};
        return s_deviceInfo(s_remaining);
    }
    return {.m_info = std::move(s_info), .m_peer_session = s_response_peer};
}

FirmwareInstallResult FirmwareUpdateSession::rebootAndWait(const Version& s_expected,
                                                          const FirmwareInstallOptions& s_options) {
    if (!s_validTimeout(s_options.m_query_timeout) || !s_validTimeout(s_options.m_reboot_timeout) ||
        !s_validTimeout(s_options.m_boot_timeout) || !s_validTimeout(s_options.m_poll_interval))
        return {.m_error = FirmwareUpdateError::kInvalidArgument};
    auto& s_client = m_endpoint.upgrade();
    const auto s_admission = s_client.beginVerification();
    if (s_admission != FirmwareUpdateError::kNone) return {.m_error = s_admission};
    struct Guard { FciUpgradeClient& m_client; ~Guard() { m_client.endVerification(); } } s_guard{s_client};
    const auto s_before = deviceInfo(s_options.m_query_timeout);
    if (!s_before) return {.m_error = s_before.m_error};
    FirmwareInstallResult s_result{};
    s_result.m_previous_info = s_before.m_info;
    if (!isProtocolVersionCompatible(s_before.m_info->m_protocol_version)) {
        s_result.m_error = FirmwareUpdateError::kProtocolMismatch;
        return s_result;
    }
    if (s_before.m_info->m_serial_number.empty() || s_before.m_info->m_board_name.empty()) {
        s_result.m_error = FirmwareUpdateError::kIdentityMismatch;
        return s_result;
    }
    // FCI exposes only major/minor/patch, not a running image digest/build ID.
    // Equal versions cannot distinguish rollback from installing a new build.
    if (s_sameVersion(s_expected, s_before.m_info->m_firmware_version)) {
        s_result.m_error = FirmwareUpdateError::kInvalidArgument;
        return s_result; // no reset issued
    }
    const auto s_previous_peer = s_before.m_peer_session;
    if (!s_previous_peer) {
        s_result.m_error = FirmwareUpdateError::kProtocolError;
        return s_result;
    }
    if (s_previous_peer != s_client.peerSession()) {
        s_result.m_error = FirmwareUpdateError::kTransportError;
        return s_result;
    }
    s_result.m_reboot = s_client.reboot(FirmwareRebootMode::kTryBoot, s_options.m_reboot_timeout, true);
    // A lost response is ambiguous: observe, never resend this mutating RPC.
    if (s_result.m_reboot.m_error != FirmwareUpdateError::kNone &&
        s_result.m_reboot.m_error != FirmwareUpdateError::kTimeout &&
        s_result.m_reboot.m_error != FirmwareUpdateError::kTransportError) {
        s_result.m_error = s_result.m_reboot.m_error;
        return s_result;
    }
    const auto s_deadline = std::chrono::steady_clock::now() + s_options.m_boot_timeout;
    const auto s_remaining = [&] {
        return std::max(0ms, std::chrono::duration_cast<std::chrono::milliseconds>(
            s_deadline - std::chrono::steady_clock::now()));
    };
    while (s_remaining() > 0ms) {
        const auto s_info = deviceInfo(std::min(s_options.m_query_timeout, s_remaining()));
        const auto s_peer = s_info.m_peer_session;
        if (s_info && s_peer && s_peer != s_previous_peer && s_peer == s_client.peerSession()) {
            s_result.m_device_info = s_info.m_info;
            s_result.m_boot_status.reset();
            const auto& s_current = *s_info.m_info;
            const auto& s_previous = *s_before.m_info;
            if (s_current.m_serial_number != s_previous.m_serial_number ||
                s_current.m_board_name != s_previous.m_board_name ||
                s_current.m_firmware_type != s_previous.m_firmware_type) {
                s_result.m_error = FirmwareUpdateError::kIdentityMismatch;
                return s_result;
            }
            if (!isProtocolVersionCompatible(s_current.m_protocol_version)) {
                s_result.m_error = FirmwareUpdateError::kProtocolMismatch;
                return s_result;
            }
            if (s_remaining() == 0ms) break;
            const auto s_boot = s_client.bootStatus(std::min(s_options.m_query_timeout, s_remaining()));
            if (s_peer == s_client.peerSession()) {
                s_result.m_boot_status = s_boot;
                if (s_boot.m_error == FirmwareUpdateError::kNone && s_boot.m_protocol_version == 1 &&
                    s_boot.m_boot_state == BOOT_IDLE && s_boot.m_active_slot == SLOT_PRIMARY &&
                    s_boot.m_confirmed_slot == SLOT_PRIMARY && s_boot.m_pending_slot == SLOT_NONE &&
                    !s_boot.m_active_transfer_id && s_boot.m_slot0_size && s_boot.m_slot1_size) {
                    if (s_sameVersion(s_current.m_firmware_version, s_expected)) {
                        s_result.m_state = FirmwareInstallState::kTargetConfirmed;
                        return s_result;
                    }
                    if (s_sameVersion(s_current.m_firmware_version, s_previous.m_firmware_version)) {
                        s_result.m_state = FirmwareInstallState::kPreviousFirmware;
                        return s_result;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::min(s_options.m_poll_interval, s_remaining()));
    }
    s_result.m_error = FirmwareUpdateError::kTimeout;
    return s_result;
}
} // namespace florid::detail
