#include "florid/FirmwareUpdater.hpp"
#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/AstrialBulkTransport.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"
#include "florid/detail/FirmwareUpdateSession.hpp"
#include <astrial/Usb.hpp>

namespace florid {
struct FirmwareUpdater::Impl {
    // Shared Arm views keep the existing session alive; they never open USB.
    std::shared_ptr<ArmImpl> m_arm;
    std::unique_ptr<AstrialBulkTransport> m_transport;
    std::unique_ptr<detail::FciWirelinkEndpoint> m_endpoint;
    detail::FirmwareUpdateSession* m_session{};
    detail::FciUpgradeClient* m_client{};
    ~Impl() { if (m_endpoint) m_endpoint->stop(); }
};

FirmwareUpdater::FirmwareUpdater() : m_impl(std::make_unique<Impl>()) {}
FirmwareUpdater::FirmwareUpdater(std::shared_ptr<ArmImpl> s_arm) : FirmwareUpdater() {
    m_impl->m_arm = std::move(s_arm);
    if (m_impl->m_arm) {
        m_impl->m_client = &m_impl->m_arm->upgradeClient();
        m_impl->m_session = &m_impl->m_arm->firmwareEndpoint().firmwareSession();
    }
}
FirmwareUpdater::~FirmwareUpdater() = default;
FirmwareUpdater::FirmwareUpdater(FirmwareUpdater&&) noexcept = default;
FirmwareUpdater& FirmwareUpdater::operator=(FirmwareUpdater&&) noexcept = default;

std::unique_ptr<FirmwareUpdater> FirmwareUpdater::create(const std::string& s_usb_uri) {
    return connect(s_usb_uri).m_updater;
}

namespace {
bool s_validTimeout(std::chrono::milliseconds s_timeout) {
    return s_timeout.count() > 0 && s_timeout.count() < 0x80000000LL;
}
FirmwareConnectionError s_discoveryError(DeviceDiscoveryError s_error) {
    switch (s_error) {
    case DeviceDiscoveryError::kNone: return FirmwareConnectionError::kNone;
    case DeviceDiscoveryError::kInvalidSelector: return FirmwareConnectionError::kInvalidSelector;
    case DeviceDiscoveryError::kEnumerationFailed: return FirmwareConnectionError::kEnumerationFailed;
    case DeviceDiscoveryError::kDeviceNotFound: return FirmwareConnectionError::kDeviceNotFound;
    case DeviceDiscoveryError::kAmbiguous: return FirmwareConnectionError::kAmbiguous;
    case DeviceDiscoveryError::kTimeout: return FirmwareConnectionError::kTimeout;
    }
    return FirmwareConnectionError::kProbeFailed;
}
}

FirmwareConnectionResult FirmwareUpdater::connect(const std::string& s_usb_uri,
                                                   std::chrono::milliseconds s_timeout) {
    if (!s_validTimeout(s_timeout)) return {.m_error = FirmwareConnectionError::kInvalidSelector,
        .m_error_message = "query timeout must be in (0, 2^31) milliseconds"};
    const auto s_selection = resolveUsbBulkDevice(s_usb_uri);
    if (!s_selection) {
        FirmwareConnectionError s_error = FirmwareConnectionError::kDeviceNotFound;
        switch (s_selection.m_error) {
        case UsbDiscoveryError::InvalidUri: s_error = FirmwareConnectionError::kInvalidSelector; break;
        case UsbDiscoveryError::EnumerationFailed: s_error = FirmwareConnectionError::kEnumerationFailed; break;
        case UsbDiscoveryError::Ambiguous: s_error = FirmwareConnectionError::kAmbiguous; break;
        default: break;
        }
        return {.m_error = s_error, .m_system_error = s_selection.m_system_error,
                .m_error_message = usbDiscoveryErrorMessage(s_selection.m_error)};
    }
    return connect(DeviceDescriptor{.m_usb = *s_selection.m_device}, s_timeout);
}

FirmwareConnectionResult FirmwareUpdater::connect(const DeviceSelector& s_selector,
                                                   std::chrono::milliseconds s_timeout) {
    if (!s_validTimeout(s_timeout) || (s_selector.m_serial_number && s_selector.m_serial_number->empty()) ||
        (s_selector.m_custom_name && s_selector.m_custom_name->empty()))
        return {.m_error = FirmwareConnectionError::kInvalidSelector, .m_error_message = "invalid selector or query timeout"};
    const auto s_devices = discoverDevices({
        .m_probe = s_selector.m_custom_name.has_value() || s_selector.m_firmware_type.has_value(),
        .m_probe_timeout = s_timeout});
    if (!s_devices) return {.m_error = s_discoveryError(s_devices.m_error),
        .m_system_error = s_devices.m_system_error, .m_error_message = deviceDiscoveryErrorMessage(s_devices.m_error)};
    const auto s_selection = selectDevice(s_devices.m_devices, s_selector);
    if (!s_selection) return {.m_error = s_discoveryError(s_selection.m_error),
        .m_error_message = deviceDiscoveryErrorMessage(s_selection.m_error), .m_candidates = s_selection.m_candidates};
    auto s_result = connect(*s_selection.m_device, s_timeout);
    // Names/types may have changed since a discovery probe closed its session.
    if (s_result && !selectDevice(std::span<const DeviceDescriptor>(&*s_result.m_device, 1), s_selector)) {
        s_result.m_updater.reset();
        s_result.m_error = FirmwareConnectionError::kIdentityMismatch;
        s_result.m_error_message = "connected device no longer matches selector";
    }
    return s_result;
}

FirmwareConnectionResult FirmwareUpdater::connect(const DeviceDescriptor& s_descriptor,
                                                   std::chrono::milliseconds s_timeout) {
    const auto& s_device = s_descriptor.m_usb;
    if (!s_validTimeout(s_timeout) || !s_device.m_vendor_id || !s_device.m_product_id ||
        (s_device.m_serial_number.empty() && s_device.m_port_path.empty()))
        return {.m_error = FirmwareConnectionError::kInvalidSelector,
                .m_error_message = "descriptor requires VID/PID and an exact serial or port path; query timeout must be valid"};
    FirmwareConnectionResult s_connection{};
    s_connection.m_device = s_descriptor;
    s_connection.m_device->m_device_info.reset();
    s_connection.m_device->m_access = DeviceAccessStatus::kNotProbed;
    s_connection.m_device->m_compatibility = DeviceCompatibility::kUnknown;
    s_connection.m_device->m_system_error.clear();
    s_connection.m_device->m_error_message.clear();
    const auto s_finish = [&]() -> FirmwareConnectionResult {
        auto& s_device_result = *s_connection.m_device;
        s_device_result.m_system_error = s_connection.m_system_error;
        s_device_result.m_error_message = s_connection.m_error_message;
        if (s_connection.m_error != FirmwareConnectionError::kNone &&
            s_device_result.m_compatibility == DeviceCompatibility::kUnknown)
            s_device_result.m_compatibility = DeviceCompatibility::kProbeFailed;
        return std::move(s_connection);
    };
    try {
        auto s_result = std::unique_ptr<FirmwareUpdater>(new FirmwareUpdater());
        auto& s_impl = *s_result->m_impl;
        s_impl.m_transport = std::make_unique<AstrialBulkTransport>(s_device.m_vendor_id,
            s_device.m_product_id, s_device.m_serial_number, s_device.m_port_path);
        s_impl.m_endpoint = std::make_unique<detail::FciWirelinkEndpoint>();
        if (s_impl.m_endpoint->initialize() != detail::FciEndpointStatus::kOk ||
            s_impl.m_endpoint->attachDirectTransport(*s_impl.m_transport) != detail::FciEndpointStatus::kOk ||
            s_impl.m_endpoint->start() != detail::FciEndpointStatus::kOk) {
            s_connection.m_system_error = s_impl.m_transport->lastError();
            s_connection.m_error = s_connection.m_system_error == make_error_code(UsbError::PermissionDenied)
                ? FirmwareConnectionError::kPermissionDenied
                : s_connection.m_system_error == make_error_code(UsbError::InterfaceBusy)
                ? FirmwareConnectionError::kBusy : FirmwareConnectionError::kTransportError;
            s_connection.m_device->m_access = s_connection.m_error == FirmwareConnectionError::kPermissionDenied
                ? DeviceAccessStatus::kPermissionDenied : s_connection.m_error == FirmwareConnectionError::kBusy
                ? DeviceAccessStatus::kBusy : DeviceAccessStatus::kUnavailable;
            s_connection.m_error_message = s_connection.m_system_error
                ? s_connection.m_system_error.message() : "Wirelink endpoint initialization/start failed";
            return s_finish();
        }
        s_connection.m_device->m_access = DeviceAccessStatus::kReady;
        s_impl.m_client = &s_impl.m_endpoint->upgrade();
        s_impl.m_session = &s_impl.m_endpoint->firmwareSession();
        const auto s_info = s_result->deviceInfo(s_timeout);
        if (!s_info) {
            s_connection.m_probe_error = s_info.m_error;
            s_connection.m_device_status = s_info.m_device_status;
            s_connection.m_error = s_info.m_error == FirmwareUpdateError::kTimeout
                ? FirmwareConnectionError::kTimeout : FirmwareConnectionError::kProbeFailed;
            s_connection.m_error_message = "same-session GetDeviceInfo probe failed";
            return s_finish();
        }
        auto& s_probed = *s_connection.m_device;
        s_probed.m_access = DeviceAccessStatus::kReady;
        s_probed.m_device_info = s_info.m_info;
        s_probed.m_system_error.clear();
        s_probed.m_error_message.clear();
        const bool s_usb_mismatch = !s_device.m_serial_number.empty() &&
            s_device.m_serial_number != s_info.m_info->m_serial_number;
        const auto& s_cached = s_descriptor.m_device_info;
        if (s_usb_mismatch || (s_cached &&
            (s_cached->m_serial_number != s_info.m_info->m_serial_number ||
             s_cached->m_board_name != s_info.m_info->m_board_name ||
             s_cached->m_firmware_type != s_info.m_info->m_firmware_type))) {
            s_probed.m_compatibility = DeviceCompatibility::kIdentityMismatch;
            s_connection.m_error = FirmwareConnectionError::kIdentityMismatch;
            s_connection.m_error_message = "USB/discovered identity differs from connected firmware";
        } else if (!isProtocolVersionCompatible(s_info.m_info->m_protocol_version)) {
            s_probed.m_compatibility = DeviceCompatibility::kProtocolMismatch;
            s_connection.m_error = FirmwareConnectionError::kProtocolMismatch;
            s_connection.m_error_message = "unsupported FCI protocol version";
        } else {
            s_probed.m_compatibility = DeviceCompatibility::kCompatible;
            s_probed.m_display_name = s_info.m_info->m_custom_name.empty()
                ? s_device.m_display_name : s_info.m_info->m_custom_name;
            s_connection.m_updater = std::move(s_result);
        }
    } catch (const std::system_error& s_error) {
        s_connection.m_error = FirmwareConnectionError::kTransportError;
        s_connection.m_system_error = s_error.code();
        s_connection.m_error_message = s_error.what();
    } catch (const std::exception& s_error) {
        s_connection.m_error = FirmwareConnectionError::kTransportError;
        s_connection.m_error_message = s_error.what();
    }
    return s_finish();
}

FirmwareDeviceInfoResult FirmwareUpdater::deviceInfo(std::chrono::milliseconds s_timeout) {
    return m_impl && m_impl->m_session ? m_impl->m_session->deviceInfo(s_timeout)
        : FirmwareDeviceInfoResult{.m_error = FirmwareUpdateError::kNotConnected};
}

FirmwareInstallResult FirmwareUpdater::rebootAndWait(const Version& s_expected_version,
                                                    const FirmwareInstallOptions& s_options) {
    if (m_impl && m_impl->m_arm && !m_impl->m_arm->firmwareUploadAllowed())
        return {.m_error = FirmwareUpdateError::kBusy};
    return m_impl && m_impl->m_session ? m_impl->m_session->rebootAndWait(s_expected_version, s_options)
        : FirmwareInstallResult{.m_error = FirmwareUpdateError::kNotConnected};
}

BootStatus FirmwareUpdater::bootStatus(std::chrono::milliseconds s_timeout) {
    return m_impl && m_impl->m_client ? m_impl->m_client->bootStatus(s_timeout)
        : BootStatus{.m_error = FirmwareUpdateError::kNotConnected};
}
FirmwareRebootResult FirmwareUpdater::reboot(FirmwareRebootMode s_mode, std::chrono::milliseconds s_timeout) {
    if (m_impl && m_impl->m_arm && !m_impl->m_arm->firmwareUploadAllowed())
        return {.m_error = FirmwareUpdateError::kBusy};
    return m_impl && m_impl->m_client ? m_impl->m_client->reboot(s_mode, s_timeout)
        : FirmwareRebootResult{.m_error = FirmwareUpdateError::kNotConnected};
}
FirmwareUpdateError FirmwareUpdater::startUpload(std::span<const std::uint8_t> s_image,
                                                const FirmwareUploadOptions& s_options) {
    if (m_impl && m_impl->m_arm && !m_impl->m_arm->firmwareUploadAllowed())
        return FirmwareUpdateError::kBusy;
    return m_impl && m_impl->m_client ? m_impl->m_client->startUpload(s_image, s_options)
        : FirmwareUpdateError::kNotConnected;
}
FirmwareUploadProgress FirmwareUpdater::progress() const {
    return m_impl && m_impl->m_client ? m_impl->m_client->progress()
        : FirmwareUploadProgress{.m_state = FirmwareUploadState::kFailed, .m_error = FirmwareUpdateError::kNotConnected};
}
void FirmwareUpdater::cancel() { if (m_impl && m_impl->m_client) m_impl->m_client->cancel(); }
} // namespace florid
