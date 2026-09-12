#include "florid/UnderwaterTeleop.hpp"
#include "florid/UsbDiscovery.hpp"
#include "florid/detail/AstrialBulkTransport.hpp"
#include "florid/detail/AstrialUSBTransport.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"
#include <mutex>
#include <atomic>
#include <thread>

namespace florid {
namespace {
using detail::FciEndpointStatus;
UnderwaterRpcError s_error(FciEndpointStatus s_status) {
    switch (s_status) {
    case FciEndpointStatus::kOk: return UnderwaterRpcError::kNone;
    case FciEndpointStatus::kNotReady: return UnderwaterRpcError::kNotConnected;
    case FciEndpointStatus::kInvalidArgument: return UnderwaterRpcError::kInvalidArgument;
    case FciEndpointStatus::kBusy:
    case FciEndpointStatus::kQueueFull: return UnderwaterRpcError::kBusy;
    case FciEndpointStatus::kTimeout: return UnderwaterRpcError::kTimeout;
    case FciEndpointStatus::kDomainError: return UnderwaterRpcError::kDeviceRejected;
    case FciEndpointStatus::kCancelled: return UnderwaterRpcError::kCancelled;
    case FciEndpointStatus::kLinkError: return UnderwaterRpcError::kTransportError;
    default: return UnderwaterRpcError::kProtocolError;
    }
}
bool s_validTimeout(std::chrono::milliseconds s_timeout) {
    return s_timeout.count() > 0 && s_timeout.count() < 0x80000000LL;
}
}

struct UnderwaterTeleop::Impl {
    std::unique_ptr<Transport> m_transport;
    detail::FciWirelinkEndpoint m_endpoint;
    std::mutex m_rpc_mutex;
    std::uint64_t m_pending_request{};
    std::atomic<std::uint64_t> m_control_session{};
    ~Impl() { m_endpoint.stop(); }

    template <typename Result, typename Submit, typename Take>
    Result transact(std::chrono::milliseconds s_timeout, Submit s_submit_rpc, Take s_take) {
        if (!s_validTimeout(s_timeout)) return {.m_error = UnderwaterRpcError::kInvalidArgument};
        std::lock_guard s_lock(m_rpc_mutex);
        detail::FciOperationResult s_operation{};
        // A local wait can expire just before the owner's terminal timeout.
        // Retain/reap that single request rather than leaking RPC slots.
        if (m_pending_request) {
            if (m_endpoint.takeOperation(m_pending_request, s_operation) == FciEndpointStatus::kBusy)
                return {.m_error = UnderwaterRpcError::kBusy};
            m_pending_request = 0;
        }
        const auto s_submit = s_submit_rpc(static_cast<std::uint32_t>(s_timeout.count()));
        if (s_submit.m_status != FciEndpointStatus::kOk)
            return {.m_error = s_error(s_submit.m_status)};
        m_pending_request = s_submit.m_request_id;
        (void)m_endpoint.waitOperation(m_pending_request, s_timeout, s_operation);
        Result s_result{};
        const auto s_status = s_take(m_pending_request, s_operation, s_result);
        if (s_status == FciEndpointStatus::kBusy)
            return {.m_error = UnderwaterRpcError::kTimeout};
        m_pending_request = 0;
        s_result.m_error = s_error(s_status);
        s_result.m_device_status = s_operation.m_domain_status;
        return s_result;
    }

    UnderwaterRpcResult call(std::optional<UnderwaterMode> s_mode,
                             std::chrono::milliseconds s_timeout, bool s_zeroing = false) {
        return transact<UnderwaterRpcResult>(s_timeout, [&](std::uint32_t s_ms) {
            if (s_zeroing) return m_endpoint.zeroing(s_ms);
            return s_mode ? m_endpoint.setUnderwaterMode(*s_mode, s_ms)
                          : m_endpoint.getUnderwaterStatus(s_ms);
        }, [&](std::uint64_t s_id, detail::FciOperationResult& s_operation, UnderwaterRpcResult& s_result) {
            UnderwaterStatus s_info{};
            bool s_has_info{};
            const auto s_status = m_endpoint.takeUnderwaterStatus(s_id, s_operation, s_info, s_has_info);
            if (s_has_info) s_result.m_info = std::move(s_info);
            return s_status;
        });
    }

    UnderwaterSettingsRpcResult settings(const UnderwaterSettings* s_settings,
        bool s_defaults, std::chrono::milliseconds s_timeout) {
        return transact<UnderwaterSettingsRpcResult>(s_timeout, [&](std::uint32_t s_ms) {
            if (s_settings) return m_endpoint.setUnderwaterSettings(*s_settings, s_ms);
            return s_defaults ? m_endpoint.getUnderwaterDefaultSettings(s_ms)
                              : m_endpoint.getUnderwaterSettings(s_ms);
        }, [&](std::uint64_t s_id, detail::FciOperationResult& s_operation, UnderwaterSettingsRpcResult& s_result) {
            return m_endpoint.takeUnderwaterSettings(s_id, s_operation, s_result);
        });
    }
};

UnderwaterTeleop::UnderwaterTeleop() : m_impl(std::make_unique<Impl>()) {}
UnderwaterTeleop::~UnderwaterTeleop() = default;
UnderwaterTeleop::UnderwaterTeleop(UnderwaterTeleop&&) noexcept = default;
UnderwaterTeleop& UnderwaterTeleop::operator=(UnderwaterTeleop&&) noexcept = default;
void UnderwaterTeleop::close() { m_impl.reset(); }

UnderwaterConnectionResult UnderwaterTeleop::s_connectTransport(
    std::unique_ptr<Transport> s_transport, std::chrono::milliseconds s_timeout) {
    if (!s_validTimeout(s_timeout))
        return {.m_error = UnderwaterRpcError::kInvalidArgument, .m_message = "invalid query timeout"};
    auto s_device = std::unique_ptr<UnderwaterTeleop>(new UnderwaterTeleop());
    auto& s_impl = *s_device->m_impl;
    s_impl.m_transport = std::move(s_transport);
    if (!s_impl.m_transport || s_impl.m_endpoint.initialize() != FciEndpointStatus::kOk ||
        s_impl.m_endpoint.attachDirectTransport(*s_impl.m_transport) != FciEndpointStatus::kOk ||
        s_impl.m_endpoint.start() != FciEndpointStatus::kOk)
        return {.m_error = UnderwaterRpcError::kTransportError,
                .m_system_error = s_impl.m_transport ? s_impl.m_transport->lastError() : std::error_code{},
                .m_message = "Wirelink transport initialization/start failed"};
    auto s_probe = s_device->status(s_timeout);
    if (!s_probe)
        return {.m_error = s_probe.m_error == UnderwaterRpcError::kNone
                    ? UnderwaterRpcError::kProtocolError : s_probe.m_error,
                .m_message = "GetUnderwaterStatus probe failed; verify the underwater firmware and USB interface",
                .m_probe = std::move(s_probe)};
    return {.m_device = std::move(s_device), .m_probe = std::move(s_probe)};
}

UnderwaterConnectionResult UnderwaterTeleop::connect(
    const DeviceDescriptor& s_device, std::chrono::milliseconds s_timeout) {
    const auto& s_usb = s_device.m_usb;
    if (!s_usb.m_vendor_id || !s_usb.m_product_id ||
        (s_usb.m_serial_number.empty() && s_usb.m_port_path.empty()))
        return {.m_error = UnderwaterRpcError::kInvalidArgument,
                .m_message = "descriptor requires VID/PID and exact serial or port path"};
    try {
        return s_connectTransport(std::make_unique<AstrialBulkTransport>(
            s_usb.m_vendor_id, s_usb.m_product_id, s_usb.m_serial_number, s_usb.m_port_path), s_timeout);
    } catch (const std::system_error& s_error) {
        return {.m_error = UnderwaterRpcError::kTransportError,
                .m_system_error = s_error.code(), .m_message = s_error.what()};
    } catch (const std::exception& s_error) {
        return {.m_error = UnderwaterRpcError::kTransportError, .m_message = s_error.what()};
    }
}

UnderwaterConnectionResult UnderwaterTeleop::connect(
    const std::string& s_uri, std::chrono::milliseconds s_timeout) {
    if (!s_validTimeout(s_timeout))
        return {.m_error = UnderwaterRpcError::kInvalidArgument, .m_message = "invalid query timeout"};
    if (s_uri.starts_with("usb://")) {
        const auto s_selection = resolveUsbBulkDevice(s_uri);
        if (!s_selection)
            return {.m_error = s_selection.m_error == UsbDiscoveryError::InvalidUri
                        ? UnderwaterRpcError::kInvalidArgument : UnderwaterRpcError::kTransportError,
                    .m_system_error = s_selection.m_system_error,
                    .m_message = usbDiscoveryErrorMessage(s_selection.m_error)};
        return connect(DeviceDescriptor{.m_usb = *s_selection.m_device}, s_timeout);
    }
    if (s_uri.starts_with("serial://") && s_uri.size() > 9) {
        try {
            return s_connectTransport(std::make_unique<AstrialUSBTransport>(s_uri.substr(9)), s_timeout);
        } catch (const std::system_error& s_error) {
            return {.m_error = UnderwaterRpcError::kTransportError,
                    .m_system_error = s_error.code(), .m_message = s_error.what()};
        } catch (const std::exception& s_error) {
            return {.m_error = UnderwaterRpcError::kTransportError, .m_message = s_error.what()};
        }
    }
    return {.m_error = UnderwaterRpcError::kInvalidArgument,
            .m_message = "use an explicit usb://VID:PID/SERIAL Bulk URI or serial:// CDC path"};
}

std::unique_ptr<UnderwaterTeleop> UnderwaterTeleop::create(const std::string& s_uri) {
    return connect(s_uri).m_device;
}
UnderwaterRpcResult UnderwaterTeleop::status(std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->call(std::nullopt, s_timeout)
                  : UnderwaterRpcResult{.m_error = UnderwaterRpcError::kNotConnected};
}
UnderwaterRpcResult UnderwaterTeleop::s_mode(UnderwaterMode s_mode, std::chrono::milliseconds s_timeout) {
    if (!m_impl) return {.m_error = UnderwaterRpcError::kNotConnected};
    auto s_result = m_impl->call(s_mode, s_timeout);
    if (s_result && s_mode != UnderwaterMode::kControl) m_impl->m_control_session.store(0, std::memory_order_release);
    return s_result;
}
UnderwaterRpcResult UnderwaterTeleop::enable(std::chrono::milliseconds s_timeout) {
    return s_mode(UnderwaterMode::kGravity, s_timeout);
}
UnderwaterRpcResult UnderwaterTeleop::disable(std::chrono::milliseconds s_timeout) {
    return s_mode(UnderwaterMode::kDamp, s_timeout);
}
UnderwaterRpcResult UnderwaterTeleop::enterControl(std::chrono::milliseconds s_timeout) {
    auto s_result = s_mode(UnderwaterMode::kControl, s_timeout);
    if (s_result && m_impl) {
        m_impl->m_control_session.store(s_result.m_info->m_control_session, std::memory_order_release);
        if (!s_result.m_info->m_control_session) s_result.m_error = UnderwaterRpcError::kProtocolError;
    }
    return s_result;
}
std::optional<UnderwaterArmStatus> UnderwaterTeleop::readArmStatus() const {
    return m_impl ? m_impl->m_endpoint.underwaterArmStatus() : std::nullopt;
}
UnderwaterCommandResult UnderwaterTeleop::writeArmCommand(const UnderwaterArmCommand& s_command) {
    if (!m_impl) return {.m_error = UnderwaterRpcError::kNotConnected};
    UnderwaterCommandResult s_result{};
    s_result.m_error = s_error(m_impl->m_endpoint.sendUnderwaterArmCommand(s_command,
        m_impl->m_control_session.load(std::memory_order_acquire), s_result.m_sequence));
    return s_result;
}
UnderwaterRpcResult UnderwaterTeleop::zeroing(std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->call(std::nullopt, s_timeout, true)
                  : UnderwaterRpcResult{.m_error = UnderwaterRpcError::kNotConnected};
}
UnderwaterRpcResult UnderwaterTeleop::enterTeleop(std::chrono::milliseconds s_timeout) {
    return s_mode(UnderwaterMode::kTeleop, s_timeout);
}
UnderwaterSettingsRpcResult UnderwaterTeleop::settings(std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->settings(nullptr, false, s_timeout)
                  : UnderwaterSettingsRpcResult{.m_error = UnderwaterRpcError::kNotConnected};
}
UnderwaterSettingsRpcResult UnderwaterTeleop::defaultSettings(std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->settings(nullptr, true, s_timeout)
                  : UnderwaterSettingsRpcResult{.m_error = UnderwaterRpcError::kNotConnected};
}
UnderwaterSettingsRpcResult UnderwaterTeleop::setSettings(
    const UnderwaterSettings& s_settings, std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->settings(&s_settings, false, s_timeout)
                  : UnderwaterSettingsRpcResult{.m_error = UnderwaterRpcError::kNotConnected};
}
FirmwareDeviceInfoResult UnderwaterTeleop::deviceInfo(std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->m_endpoint.firmwareSession().deviceInfo(s_timeout)
                  : FirmwareDeviceInfoResult{.m_error = FirmwareUpdateError::kNotConnected};
}
BootStatus UnderwaterTeleop::bootStatus(std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->m_endpoint.upgrade().bootStatus(s_timeout)
                  : BootStatus{.m_error = FirmwareUpdateError::kNotConnected};
}
FirmwareUpdateError UnderwaterTeleop::startFirmwareUpload(std::span<const std::uint8_t> s_image,
                                                        const FirmwareUploadOptions& s_options) {
    return m_impl ? m_impl->m_endpoint.upgrade().startUpload(s_image, s_options)
                  : FirmwareUpdateError::kNotConnected;
}
FirmwareUploadProgress UnderwaterTeleop::firmwareProgress() const {
    return m_impl ? m_impl->m_endpoint.upgrade().progress()
                  : FirmwareUploadProgress{.m_state = FirmwareUploadState::kFailed,
                                           .m_error = FirmwareUpdateError::kNotConnected};
}
void UnderwaterTeleop::cancelFirmwareUpload() {
    if (m_impl) m_impl->m_endpoint.upgrade().cancel();
}
FirmwareUploadProgress UnderwaterTeleop::uploadFirmware(std::span<const std::uint8_t> s_image,
                                                       const FirmwareUploadOptions& s_options) {
    const auto s_error = startFirmwareUpload(s_image, s_options);
    if (s_error != FirmwareUpdateError::kNone)
        return {.m_state = FirmwareUploadState::kFailed, .m_error = s_error};
    for (;;) {
        auto s_progress = firmwareProgress();
        if (!s_progress.active()) return s_progress;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
FirmwareRebootResult UnderwaterTeleop::reboot(FirmwareRebootMode s_mode,
                                            std::chrono::milliseconds s_timeout) {
    return m_impl ? m_impl->m_endpoint.upgrade().reboot(s_mode, s_timeout)
                  : FirmwareRebootResult{.m_error = FirmwareUpdateError::kNotConnected};
}
FirmwareInstallResult UnderwaterTeleop::rebootAndWait(const Version& s_expected_version,
                                                    const FirmwareInstallOptions& s_options) {
    return m_impl ? m_impl->m_endpoint.firmwareSession().rebootAndWait(s_expected_version, s_options)
                  : FirmwareInstallResult{.m_error = FirmwareUpdateError::kNotConnected};
}
UnderwaterFirmwareUpdateResult UnderwaterTeleop::updateFirmware(
    std::span<const std::uint8_t> s_image, const Version& s_expected_version,
    const FirmwareUploadOptions& s_upload_options, const FirmwareInstallOptions& s_install_options) {
    UnderwaterFirmwareUpdateResult s_result{};
    s_result.m_upload = uploadFirmware(s_image, s_upload_options);
    if (s_result.m_upload.m_state == FirmwareUploadState::kCompleted)
        s_result.m_install = rebootAndWait(s_expected_version, s_install_options);
    return s_result;
}
} // namespace florid
