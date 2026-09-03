#include "florid/UsbDiscovery.hpp"
#include "florid/DeviceDiscovery.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using florid::UsbBulkDeviceInfo;
using florid::UsbDiscoveryError;

void require(bool s_condition, const char* s_message) {
    if (!s_condition) throw std::runtime_error(s_message);
}

UsbBulkDeviceInfo s_device(std::string s_serial,
                           std::vector<std::uint8_t> s_path,
                           std::string s_product = "Ragtime Florid A1B2C3D4") {
    return UsbBulkDeviceInfo{
        .m_vendor_id = florid::kDefaultUsbVendorId,
        .m_product_id = florid::kDefaultUsbProductId,
        .m_bus_number = 1,
        .m_device_address = 2,
        .m_port_path = std::move(s_path),
        .m_manufacturer = "Ragtime",
        .m_product = std::move(s_product),
        .m_serial_number = std::move(s_serial),
        .m_display_name = {},
        .m_uri = {},
    };
}

void testZeroAndOneMatch() {
    const std::array<UsbBulkDeviceInfo, 0> s_empty{};
    const auto s_missing =
        florid::selectUsbBulkDevice(s_empty, "usb://2fe3:574c");
    require(!s_missing &&
                s_missing.m_error == UsbDiscoveryError::DeviceNotFound &&
                s_missing.m_match_count == 0,
            "empty discovery did not report device-not-found");

    const std::array s_devices{s_device("FULL-SERIAL-01", {1, 4})};
    const auto s_selected = florid::selectUsbBulkDevice(s_devices, "usb://");
    require(s_selected && s_selected.m_match_count == 1,
            "single default device was not selected");
    require(s_selected.m_device->m_display_name ==
                "Ragtime Florid A1B2C3D4",
            "USB product was not exposed as display name");
    require(s_selected.m_device->m_serial_number == "FULL-SERIAL-01",
            "full serial was not preserved");
    require(s_selected.m_device->m_uri ==
                "usb://2fe3:574c/FULL-SERIAL-01",
            "stable serial URI was not generated");
}

void testAmbiguousAndExactSerial() {
    const std::array s_devices{
        s_device("FULL/SERIAL A", {1, 4}),
        s_device("FULL-SERIAL-B", {1, 5}),
        UsbBulkDeviceInfo{.m_vendor_id = 0x1234,
                          .m_product_id = 0xabcd,
                          .m_bus_number = 3,
                          .m_device_address = 4,
                          .m_port_path = {},
                          .m_manufacturer = {},
                          .m_product = {},
                          .m_serial_number = "OTHER",
                          .m_display_name = {},
                          .m_uri = {}},
    };

    const auto s_ambiguous =
        florid::selectUsbBulkDevice(s_devices, "usb://2fe3:574c");
    require(!s_ambiguous &&
                s_ambiguous.m_error == UsbDiscoveryError::Ambiguous &&
                s_ambiguous.m_match_count == 2,
            "non-exact selector silently accepted multiple devices");

    const auto s_exact = florid::selectUsbBulkDevice(
        s_devices, "usb://2FE3:574C/FULL%2FSERIAL%20A");
    require(s_exact && s_exact.m_match_count == 1 &&
                s_exact.m_device->m_serial_number == "FULL/SERIAL A",
            "percent-encoded exact serial did not select the device");
    require(s_exact.m_device->m_uri ==
                "usb://2fe3:574c/FULL%2FSERIAL%20A",
            "exact serial URI was not canonicalized");
}

void testPortPathAndInvalidUris() {
    const std::array s_devices{
        s_device("", {2, 7}, ""),
        s_device("", {2, 8}, ""),
    };
    const auto s_exact = florid::selectUsbBulkDevice(
        s_devices, "usb://2fe3:574c?port=2.8");
    require(s_exact && s_exact.m_device->m_port_path ==
                           std::vector<std::uint8_t>({2, 8}),
            "physical port selector did not select exactly one device");
    require(s_exact.m_device->m_uri == "usb://2fe3:574c?port=2.8",
            "physical port URI was not generated");
    require(s_exact.m_device->m_display_name == "Ragtime",
            "manufacturer display-name fallback was not used");

    constexpr std::array<std::string_view, 7> s_invalid{
        "serial://COM3",       "usb://xyz:574c",
        "usb://2fe3:0",       "usb://2fe3:574c/",
        "usb://2fe3:574c/%Q0", "usb://2fe3:574c?port=",
        "usb://2fe3:574c?foo=1",
    };
    for (const auto s_uri : s_invalid) {
        const auto s_result = florid::selectUsbBulkDevice(s_devices, s_uri);
        require(!s_result && s_result.m_error == UsbDiscoveryError::InvalidUri,
                "invalid USB URI was accepted");
    }
}

florid::DeviceDescriptor s_descriptor(
    std::string s_usb_serial, std::string s_protocol_serial,
    std::string s_custom_name,
    florid::DeviceCompatibility s_compatibility =
        florid::DeviceCompatibility::kCompatible) {
    auto s_result = florid::DeviceDescriptor{};
    s_result.m_usb = s_device(std::move(s_usb_serial), {1, 6});
    s_result.m_device_info = florid::DeviceInfo{
        .m_protocol_version = florid::kSupportedProtocolVersion,
        .m_firmware_version = {1, 2, 3},
        .m_board_name = "test-board",
        .m_custom_name = std::move(s_custom_name),
        .m_firmware_type = florid::FirmwareType::kStandardArm,
        .m_serial_number = std::move(s_protocol_serial),
    };
    s_result.m_access = florid::DeviceAccessStatus::kReady;
    s_result.m_compatibility = s_compatibility;
    return s_result;
}

void testProductDeviceSelection() {
    const std::array s_devices{
        s_descriptor("USB-A", "FULL-A", "left-arm"),
        s_descriptor("USB-B", "FULL-B", "right-arm"),
    };

    const auto s_by_serial = florid::selectDevice(
        s_devices, florid::DeviceSelector::bySerial("FULL-B"));
    require(s_by_serial &&
                s_by_serial.m_device->serialNumber() == "FULL-B",
            "protocol serial did not select the device");

    const auto s_by_name = florid::selectDevice(
        s_devices, florid::DeviceSelector::byCustomName("left-arm"));
    require(s_by_name &&
                s_by_name.m_device->m_device_info->m_serial_number ==
                    "FULL-A",
            "custom name did not select the device");

    const auto s_default = florid::selectDevice(s_devices);
    require(!s_default &&
                s_default.m_error ==
                    florid::DeviceDiscoveryError::kAmbiguous &&
                s_default.m_candidates.size() == 2,
            "default selection silently chose one of several devices");
}

void testDuplicateNamesAndCompatibility() {
    const std::array s_devices{
        s_descriptor("USB-A", "FULL-A", "arm"),
        s_descriptor("USB-B", "FULL-B", "arm"),
        s_descriptor("USB-C", "FULL-C", "old-arm",
                     florid::DeviceCompatibility::kProtocolMismatch),
    };

    const auto s_duplicate = florid::selectDevice(
        s_devices, florid::DeviceSelector::byCustomName("arm"));
    require(!s_duplicate &&
                s_duplicate.m_error ==
                    florid::DeviceDiscoveryError::kAmbiguous &&
                s_duplicate.m_candidates.size() == 2,
            "duplicate custom names were treated as unique");

    const auto s_incompatible = florid::selectDevice(
        s_devices, florid::DeviceSelector::bySerial("FULL-C"));
    require(!s_incompatible &&
                s_incompatible.m_error ==
                    florid::DeviceDiscoveryError::kDeviceNotFound,
            "incompatible device was selectable");

    require(florid::isProtocolVersionCompatible({0, 0, 99}),
            "compatible protocol patch was rejected");
    require(!florid::isProtocolVersionCompatible({0, 1, 0}),
            "incompatible pre-1.0 protocol minor was accepted");
}

} // namespace

int main() {
    testZeroAndOneMatch();
    testAmbiguousAndExactSerial();
    testPortPathAndInvalidUris();
    testProductDeviceSelection();
    testDuplicateNamesAndCompatibility();
    return 0;
}
