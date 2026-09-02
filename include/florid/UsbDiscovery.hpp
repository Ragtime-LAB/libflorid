#ifndef FLORID_USB_DISCOVERY_HPP
#define FLORID_USB_DISCOVERY_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace florid {

inline constexpr std::uint16_t kDefaultUsbVendorId = 0x2fe3;
inline constexpr std::uint16_t kDefaultUsbProductId = 0x574c;

struct UsbBulkDeviceInfo {
    std::uint16_t m_vendor_id{};
    std::uint16_t m_product_id{};
    std::uint8_t m_bus_number{};
    std::uint8_t m_device_address{};
    std::vector<std::uint8_t> m_port_path;
    std::string m_manufacturer;
    std::string m_product;
    std::string m_serial_number;
    // Friendly text suitable for a device picker. This is the USB product
    // string when available and has a deterministic VID:PID fallback.
    std::string m_display_name;
    // Exact selector when the device exposes a serial number or USB port path.
    // Serial numbers are percent-encoded, so the URI can be stored verbatim.
    std::string m_uri;
};

enum class UsbDiscoveryError {
    None = 0,
    InvalidUri,
    EnumerationFailed,
    DeviceNotFound,
    Ambiguous,
};

struct UsbBulkDiscoveryResult {
    std::vector<UsbBulkDeviceInfo> m_devices;
    std::error_code m_system_error;

    [[nodiscard]] explicit operator bool() const noexcept {
        return !m_system_error;
    }
};

struct UsbBulkSelectionResult {
    std::optional<UsbBulkDeviceInfo> m_device;
    UsbDiscoveryError m_error{UsbDiscoveryError::None};
    std::size_t m_match_count{};
    std::error_code m_system_error;

    [[nodiscard]] explicit operator bool() const noexcept {
        return m_device.has_value() && m_error == UsbDiscoveryError::None;
    }
};

// Enumerates USB devices visible to the current process. Descriptor strings
// may be empty when the OS permits enumeration but not opening a device.
[[nodiscard]] UsbBulkDiscoveryResult discoverUsbBulkDevices();

// Pure selection helper for device pickers and tests. Accepted forms are:
//   usb://                    (default VID/PID)
//   usb://vvvv:pppp          (unique match required)
//   usb://vvvv:pppp/SERIAL   (exact, percent-encoded serial)
//   usb://vvvv:pppp?port=1.2 (exact physical USB port path)
[[nodiscard]] UsbBulkSelectionResult selectUsbBulkDevice(
    std::span<const UsbBulkDeviceInfo> s_devices, std::string_view s_uri);

// Convenience form used by Arm::create(): enumerate, then select.
[[nodiscard]] UsbBulkSelectionResult resolveUsbBulkDevice(
    std::string_view s_uri);

[[nodiscard]] const char* usbDiscoveryErrorMessage(
    UsbDiscoveryError s_error) noexcept;

} // namespace florid

#endif // FLORID_USB_DISCOVERY_HPP
