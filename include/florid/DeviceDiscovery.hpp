#ifndef FLORID_DEVICE_DISCOVERY_HPP
#define FLORID_DEVICE_DISCOVERY_HPP

#include "florid/DeviceTypes.hpp"
#include "florid/UsbDiscovery.hpp"

#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace florid {

enum class DeviceAccessStatus {
    kNotProbed = 0,
    kReady,
    kPermissionDenied,
    kBusy,
    kDisconnected,
    kUnavailable,
};

enum class DeviceCompatibility {
    kUnknown = 0,
    kCompatible,
    kProtocolMismatch,
    kIdentityMismatch,
    kProbeFailed,
};

enum class DeviceDiscoveryError {
    kNone = 0,
    kEnumerationFailed,
    kInvalidSelector,
    kDeviceNotFound,
    kAmbiguous,
    kTimeout,
};

enum class ArmConnectionState {
    kReady = 0,
    kDisconnected,
    kReconnecting,
    kControlUnavailable,
    kClosed,
};

struct DeviceDescriptor {
    UsbBulkDeviceInfo m_usb;
    std::optional<DeviceInfo> m_device_info;
    DeviceAccessStatus m_access{DeviceAccessStatus::kNotProbed};
    DeviceCompatibility m_compatibility{DeviceCompatibility::kUnknown};
    std::error_code m_system_error;
    std::string m_error_message;
    std::string m_display_name;

    [[nodiscard]] const std::string& uri() const noexcept {
        return m_usb.m_uri;
    }
    [[nodiscard]] const std::string& serialNumber() const noexcept {
        if (m_device_info && !m_device_info->m_serial_number.empty()) {
            return m_device_info->m_serial_number;
        }
        return m_usb.m_serial_number;
    }
};

struct DeviceSelector {
    std::optional<std::string> m_serial_number;
    std::optional<std::string> m_custom_name;
    std::optional<FirmwareType> m_firmware_type;

    [[nodiscard]] static DeviceSelector bySerial(std::string s_serial);
    [[nodiscard]] static DeviceSelector byCustomName(std::string s_name);
};

struct DeviceDiscoveryOptions {
    bool m_probe{false};
    std::chrono::milliseconds m_probe_timeout{1500};
};

struct DeviceDiscoveryResult {
    std::vector<DeviceDescriptor> m_devices;
    DeviceDiscoveryError m_error{DeviceDiscoveryError::kNone};
    std::error_code m_system_error;

    [[nodiscard]] explicit operator bool() const noexcept {
        return m_error == DeviceDiscoveryError::kNone;
    }
};

struct DeviceSelectionResult {
    std::optional<DeviceDescriptor> m_device;
    std::vector<DeviceDescriptor> m_candidates;
    DeviceDiscoveryError m_error{DeviceDiscoveryError::kNone};

    [[nodiscard]] explicit operator bool() const noexcept {
        return m_device.has_value() && m_error == DeviceDiscoveryError::kNone;
    }
};

// Fast physical discovery. When m_probe is false this only performs USB
// enumeration. Probing opens each device briefly and issues read-only
// GetDeviceInfo; it never acquires the control lease.
[[nodiscard]] DeviceDiscoveryResult discoverDevices(
    const DeviceDiscoveryOptions& s_options = {});

// Probe one previously enumerated USB device without acquiring control.
[[nodiscard]] DeviceDescriptor probeDevice(
    const UsbBulkDeviceInfo& s_device,
    std::chrono::milliseconds s_timeout = std::chrono::milliseconds{1500});

// Pure selector used by device pickers and unit tests. An empty selector means
// "the only compatible device". User-editable custom names are intentionally
// not assumed to be unique.
[[nodiscard]] DeviceSelectionResult selectDevice(
    std::span<const DeviceDescriptor> s_devices,
    const DeviceSelector& s_selector = {});

// Polling wait keeps the API portable; platform hotplug implementations can
// replace the backend later without changing this contract.
[[nodiscard]] DeviceSelectionResult waitForDevice(
    const DeviceSelector& s_selector,
    std::chrono::milliseconds s_timeout,
    std::chrono::milliseconds s_poll_interval =
        std::chrono::milliseconds{100});

[[nodiscard]] const char* deviceDiscoveryErrorMessage(
    DeviceDiscoveryError s_error) noexcept;

[[nodiscard]] bool isProtocolVersionCompatible(
    const Version& s_version) noexcept;

} // namespace florid

#endif // FLORID_DEVICE_DISCOVERY_HPP
