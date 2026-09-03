#include "florid/DeviceDiscovery.hpp"

#include "florid/detail/AstrialBulkTransport.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"

#include <astrial/Usb.hpp>

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>

namespace florid {
namespace {

using detail::FciEndpointStatus;

DeviceAccessStatus s_accessFromError(const std::error_code& s_error) {
    if (s_error == make_error_code(UsbError::PermissionDenied)) {
        return DeviceAccessStatus::kPermissionDenied;
    }
    if (s_error == make_error_code(UsbError::InterfaceBusy)) {
        return DeviceAccessStatus::kBusy;
    }
    if (s_error == make_error_code(UsbError::DeviceDisconnected) ||
        s_error == make_error_code(UsbError::DeviceNotFound)) {
        return DeviceAccessStatus::kDisconnected;
    }
    return DeviceAccessStatus::kUnavailable;
}

std::string s_displayName(const DeviceDescriptor& s_device) {
    if (s_device.m_device_info &&
        !s_device.m_device_info->m_custom_name.empty()) {
        if (!s_device.m_usb.m_display_name.empty() &&
            s_device.m_device_info->m_custom_name !=
                s_device.m_usb.m_display_name) {
            return s_device.m_device_info->m_custom_name + " (" +
                   s_device.m_usb.m_display_name + ')';
        }
        return s_device.m_device_info->m_custom_name;
    }
    return s_device.m_usb.m_display_name;
}

DeviceDescriptor s_unprobed(const UsbBulkDeviceInfo& s_usb) {
    DeviceDescriptor s_result;
    s_result.m_usb = s_usb;
    s_result.m_display_name = s_usb.m_display_name;
    return s_result;
}

void s_failProbe(DeviceDescriptor& s_result, DeviceAccessStatus s_access,
                 std::string s_message, std::error_code s_error = {}) {
    s_result.m_access = s_access;
    s_result.m_compatibility = DeviceCompatibility::kProbeFailed;
    s_result.m_system_error = s_error;
    s_result.m_error_message = std::move(s_message);
}

bool s_matches(const DeviceDescriptor& s_device,
               const DeviceSelector& s_selector) {
    if (s_device.m_compatibility != DeviceCompatibility::kUnknown &&
        s_device.m_compatibility != DeviceCompatibility::kCompatible) {
        return false;
    }
    if (s_selector.m_serial_number &&
        s_device.serialNumber() != *s_selector.m_serial_number) {
        return false;
    }
    if (s_selector.m_custom_name) {
        if (!s_device.m_device_info ||
            s_device.m_device_info->m_custom_name !=
                *s_selector.m_custom_name) {
            return false;
        }
    }
    if (s_selector.m_firmware_type) {
        if (!s_device.m_device_info ||
            s_device.m_device_info->m_firmware_type !=
                *s_selector.m_firmware_type) {
            return false;
        }
    }
    return true;
}

} // namespace

DeviceSelector DeviceSelector::bySerial(std::string s_serial) {
    DeviceSelector s_selector;
    s_selector.m_serial_number = std::move(s_serial);
    return s_selector;
}

DeviceSelector DeviceSelector::byCustomName(std::string s_name) {
    DeviceSelector s_selector;
    s_selector.m_custom_name = std::move(s_name);
    return s_selector;
}

DeviceDescriptor probeDevice(const UsbBulkDeviceInfo& s_device,
                             std::chrono::milliseconds s_timeout) {
    auto s_result = s_unprobed(s_device);
    if (s_timeout <= std::chrono::milliseconds::zero()) {
        s_failProbe(s_result, DeviceAccessStatus::kUnavailable,
                    "probe timeout must be positive");
        return s_result;
    }

    AstrialBulkTransport s_transport(
        s_device.m_vendor_id, s_device.m_product_id,
        s_device.m_serial_number, s_device.m_port_path);
    detail::FciWirelinkEndpoint s_endpoint;

    auto s_status = s_endpoint.initialize();
    if (s_status != FciEndpointStatus::kOk) {
        s_failProbe(s_result, DeviceAccessStatus::kUnavailable,
                    "Wirelink probe initialization failed");
        return s_result;
    }
    s_status = s_endpoint.attachDirectTransport(s_transport);
    if (s_status != FciEndpointStatus::kOk) {
        const auto s_error = s_transport.lastError();
        s_failProbe(s_result, s_accessFromError(s_error),
                    s_error ? s_error.message() : "USB interface open failed",
                    s_error);
        return s_result;
    }
    s_status = s_endpoint.start();
    if (s_status != FciEndpointStatus::kOk) {
        s_failProbe(s_result, DeviceAccessStatus::kUnavailable,
                    "Wirelink probe start failed");
        return s_result;
    }

    s_result.m_access = DeviceAccessStatus::kReady;
    const auto s_rpc_timeout = static_cast<std::uint32_t>(
        std::max<std::int64_t>(1, s_timeout.count()));
    const auto s_submit = s_endpoint.getDeviceInfo(s_rpc_timeout);
    if (s_submit.m_status != FciEndpointStatus::kOk) {
        s_failProbe(s_result, DeviceAccessStatus::kReady,
                    "GetDeviceInfo submission failed");
        return s_result;
    }

    detail::FciOperationResult s_operation{};
    const auto s_wait =
        s_endpoint.waitOperation(s_submit.m_request_id, s_timeout, s_operation);
    if (s_wait != FciEndpointStatus::kOk) {
        s_failProbe(s_result, DeviceAccessStatus::kReady,
                    s_wait == FciEndpointStatus::kBusy
                        ? "GetDeviceInfo probe timed out"
                        : "GetDeviceInfo probe failed");
        return s_result;
    }

    DeviceInfo s_info{};
    const auto s_take = s_endpoint.takeDeviceInfo(
        s_submit.m_request_id, s_operation, s_info);
    if (s_take != FciEndpointStatus::kOk) {
        s_failProbe(s_result, DeviceAccessStatus::kReady,
                    "GetDeviceInfo response was invalid");
        return s_result;
    }

    s_result.m_device_info = std::move(s_info);
    s_result.m_error_message.clear();
    if (!s_device.m_serial_number.empty() &&
        !s_result.m_device_info->m_serial_number.empty() &&
        s_device.m_serial_number !=
            s_result.m_device_info->m_serial_number) {
        s_result.m_compatibility = DeviceCompatibility::kIdentityMismatch;
        s_result.m_error_message =
            "USB serial and firmware serial do not match";
    } else if (!isProtocolVersionCompatible(
                   s_result.m_device_info->m_protocol_version)) {
        s_result.m_compatibility = DeviceCompatibility::kProtocolMismatch;
        s_result.m_error_message = "unsupported FCI protocol version";
    } else {
        s_result.m_compatibility = DeviceCompatibility::kCompatible;
    }
    s_result.m_display_name = s_displayName(s_result);
    return s_result;
}

DeviceDiscoveryResult discoverDevices(
    const DeviceDiscoveryOptions& s_options) {
    DeviceDiscoveryResult s_result;
    const auto s_usb = discoverUsbBulkDevices();
    if (!s_usb) {
        s_result.m_error = DeviceDiscoveryError::kEnumerationFailed;
        s_result.m_system_error = s_usb.m_system_error;
        return s_result;
    }

    s_result.m_devices.reserve(s_usb.m_devices.size());
    for (const auto& s_device : s_usb.m_devices) {
        if (s_options.m_probe) {
            s_result.m_devices.push_back(
                probeDevice(s_device, s_options.m_probe_timeout));
        } else {
            s_result.m_devices.push_back(s_unprobed(s_device));
        }
    }
    return s_result;
}

DeviceSelectionResult selectDevice(
    std::span<const DeviceDescriptor> s_devices,
    const DeviceSelector& s_selector) {
    DeviceSelectionResult s_result;
    if ((s_selector.m_serial_number &&
         s_selector.m_serial_number->empty()) ||
        (s_selector.m_custom_name && s_selector.m_custom_name->empty())) {
        s_result.m_error = DeviceDiscoveryError::kInvalidSelector;
        return s_result;
    }
    for (const auto& s_device : s_devices) {
        if (s_matches(s_device, s_selector)) {
            s_result.m_candidates.push_back(s_device);
        }
    }
    if (s_result.m_candidates.empty()) {
        s_result.m_error = DeviceDiscoveryError::kDeviceNotFound;
    } else if (s_result.m_candidates.size() > 1) {
        s_result.m_error = DeviceDiscoveryError::kAmbiguous;
    } else {
        s_result.m_device = s_result.m_candidates.front();
    }
    return s_result;
}

DeviceSelectionResult waitForDevice(
    const DeviceSelector& s_selector, std::chrono::milliseconds s_timeout,
    std::chrono::milliseconds s_poll_interval) {
    if (s_timeout < std::chrono::milliseconds::zero() ||
        s_poll_interval <= std::chrono::milliseconds::zero()) {
        DeviceSelectionResult s_result;
        s_result.m_error = DeviceDiscoveryError::kInvalidSelector;
        return s_result;
    }

    const auto s_deadline = std::chrono::steady_clock::now() + s_timeout;
    const bool s_needs_probe = s_selector.m_custom_name.has_value() ||
                               s_selector.m_firmware_type.has_value();
    do {
        const auto s_discovery = discoverDevices({.m_probe = s_needs_probe});
        if (!s_discovery) {
            DeviceSelectionResult s_result;
            s_result.m_error = s_discovery.m_error;
            return s_result;
        }
        auto s_selection = selectDevice(s_discovery.m_devices, s_selector);
        if (s_selection ||
            s_selection.m_error == DeviceDiscoveryError::kAmbiguous) {
            return s_selection;
        }
        const auto s_now = std::chrono::steady_clock::now();
        if (s_now >= s_deadline) break;
        std::this_thread::sleep_for(
            std::min(s_poll_interval,
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                         s_deadline - s_now)));
    } while (true);

    DeviceSelectionResult s_result;
    s_result.m_error = DeviceDiscoveryError::kTimeout;
    return s_result;
}

const char* deviceDiscoveryErrorMessage(
    DeviceDiscoveryError s_error) noexcept {
    switch (s_error) {
        case DeviceDiscoveryError::kNone: return "no error";
        case DeviceDiscoveryError::kEnumerationFailed:
            return "USB enumeration failed";
        case DeviceDiscoveryError::kInvalidSelector:
            return "invalid device selector";
        case DeviceDiscoveryError::kDeviceNotFound:
            return "device not found";
        case DeviceDiscoveryError::kAmbiguous:
            return "selector matches multiple devices";
        case DeviceDiscoveryError::kTimeout:
            return "timed out waiting for device";
    }
    return "unknown device discovery error";
}

bool isProtocolVersionCompatible(const Version& s_version) noexcept {
    if (kSupportedProtocolVersion.m_major == 0) {
        return s_version.m_major == kSupportedProtocolVersion.m_major &&
               s_version.m_minor == kSupportedProtocolVersion.m_minor;
    }
    return s_version.m_major == kSupportedProtocolVersion.m_major;
}

} // namespace florid
