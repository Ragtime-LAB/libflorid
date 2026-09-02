#include "florid/UsbDiscovery.hpp"

#include <astrial/Usb.hpp>

#include <array>
#include <charconv>
#include <cstdio>
#include <utility>

namespace florid {
namespace {

struct UsbSelector {
    std::uint16_t m_vendor_id{kDefaultUsbVendorId};
    std::uint16_t m_product_id{kDefaultUsbProductId};
    std::optional<std::string> m_serial_number;
    std::optional<std::vector<std::uint8_t>> m_port_path;
};

bool s_parseHex16(std::string_view s_text, std::uint16_t& s_value) {
    if (s_text.starts_with("0x") || s_text.starts_with("0X")) {
        s_text.remove_prefix(2);
    }
    if (s_text.empty()) return false;
    unsigned int s_parsed{};
    const auto [s_end, s_error] = std::from_chars(
        s_text.data(), s_text.data() + s_text.size(), s_parsed, 16);
    if (s_error != std::errc{} || s_end != s_text.data() + s_text.size() ||
        s_parsed == 0 || s_parsed > UINT16_MAX) {
        return false;
    }
    s_value = static_cast<std::uint16_t>(s_parsed);
    return true;
}

int s_hexDigit(char s_value) noexcept {
    if (s_value >= '0' && s_value <= '9') return s_value - '0';
    if (s_value >= 'a' && s_value <= 'f') return s_value - 'a' + 10;
    if (s_value >= 'A' && s_value <= 'F') return s_value - 'A' + 10;
    return -1;
}

bool s_percentDecode(std::string_view s_text, std::string& s_result) {
    s_result.clear();
    s_result.reserve(s_text.size());
    for (std::size_t s_index = 0; s_index < s_text.size(); ++s_index) {
        if (s_text[s_index] != '%') {
            s_result.push_back(s_text[s_index]);
            continue;
        }
        if (s_index + 2 >= s_text.size()) return false;
        const int s_high = s_hexDigit(s_text[s_index + 1]);
        const int s_low = s_hexDigit(s_text[s_index + 2]);
        if (s_high < 0 || s_low < 0) return false;
        const char s_decoded = static_cast<char>((s_high << 4) | s_low);
        if (s_decoded == '\0') return false;
        s_result.push_back(s_decoded);
        s_index += 2;
    }
    return !s_result.empty();
}

bool s_isUnreserved(unsigned char s_value) noexcept {
    return (s_value >= 'a' && s_value <= 'z') ||
           (s_value >= 'A' && s_value <= 'Z') ||
           (s_value >= '0' && s_value <= '9') || s_value == '-' ||
           s_value == '.' || s_value == '_' || s_value == '~';
}

std::string s_percentEncode(std::string_view s_text) {
    constexpr char s_hex[] = "0123456789ABCDEF";
    std::string s_result;
    s_result.reserve(s_text.size());
    for (const unsigned char s_value : s_text) {
        if (s_isUnreserved(s_value)) {
            s_result.push_back(static_cast<char>(s_value));
        } else {
            s_result.push_back('%');
            s_result.push_back(s_hex[s_value >> 4]);
            s_result.push_back(s_hex[s_value & 0x0f]);
        }
    }
    return s_result;
}

bool s_parsePortPath(std::string_view s_text,
                     std::vector<std::uint8_t>& s_result) {
    s_result.clear();
    while (!s_text.empty()) {
        const auto s_dot = s_text.find('.');
        const auto s_component = s_text.substr(0, s_dot);
        unsigned int s_value{};
        const auto [s_end, s_error] = std::from_chars(
            s_component.data(), s_component.data() + s_component.size(),
            s_value, 10);
        if (s_component.empty() || s_error != std::errc{} ||
            s_end != s_component.data() + s_component.size() ||
            s_value == 0 || s_value > UINT8_MAX) {
            return false;
        }
        s_result.push_back(static_cast<std::uint8_t>(s_value));
        if (s_dot == std::string_view::npos) break;
        s_text.remove_prefix(s_dot + 1);
    }
    return !s_result.empty();
}

std::optional<UsbSelector> s_parseSelector(std::string_view s_uri) {
    if (!s_uri.starts_with("usb://")) return std::nullopt;
    s_uri.remove_prefix(6);

    UsbSelector s_selector;
    const auto s_suffix = s_uri.find_first_of("/?");
    const auto s_ids = s_uri.substr(0, s_suffix);
    if (!s_ids.empty()) {
        const auto s_colon = s_ids.find(':');
        if (s_colon == std::string_view::npos ||
            !s_parseHex16(s_ids.substr(0, s_colon), s_selector.m_vendor_id) ||
            !s_parseHex16(s_ids.substr(s_colon + 1),
                          s_selector.m_product_id)) {
            return std::nullopt;
        }
    }
    if (s_suffix == std::string_view::npos) return s_selector;

    const char s_separator = s_uri[s_suffix];
    const auto s_value = s_uri.substr(s_suffix + 1);
    if (s_separator == '/') {
        std::string s_serial;
        if (s_value.find('?') != std::string_view::npos ||
            !s_percentDecode(s_value, s_serial)) {
            return std::nullopt;
        }
        s_selector.m_serial_number = std::move(s_serial);
        return s_selector;
    }

    constexpr std::string_view s_port_prefix = "port=";
    if (!s_value.starts_with(s_port_prefix)) return std::nullopt;
    std::vector<std::uint8_t> s_path;
    if (!s_parsePortPath(s_value.substr(s_port_prefix.size()), s_path)) {
        return std::nullopt;
    }
    s_selector.m_port_path = std::move(s_path);
    return s_selector;
}

std::string s_baseUri(std::uint16_t s_vendor_id,
                      std::uint16_t s_product_id) {
    std::array<char, 24> s_buffer{};
    std::snprintf(s_buffer.data(), s_buffer.size(), "usb://%04x:%04x",
                  s_vendor_id, s_product_id);
    return s_buffer.data();
}

void s_completeDeviceInfo(UsbBulkDeviceInfo& s_device) {
    if (!s_device.m_product.empty()) {
        s_device.m_display_name = s_device.m_product;
    } else if (!s_device.m_manufacturer.empty()) {
        s_device.m_display_name = s_device.m_manufacturer;
    } else {
        std::array<char, 32> s_buffer{};
        std::snprintf(s_buffer.data(), s_buffer.size(), "USB device %04x:%04x",
                      s_device.m_vendor_id, s_device.m_product_id);
        s_device.m_display_name = s_buffer.data();
    }

    s_device.m_uri = s_baseUri(s_device.m_vendor_id, s_device.m_product_id);
    if (!s_device.m_serial_number.empty()) {
        s_device.m_uri += '/';
        s_device.m_uri += s_percentEncode(s_device.m_serial_number);
    } else if (!s_device.m_port_path.empty()) {
        s_device.m_uri += "?port=";
        for (std::size_t s_index = 0; s_index < s_device.m_port_path.size();
             ++s_index) {
            if (s_index != 0) s_device.m_uri += '.';
            s_device.m_uri += std::to_string(s_device.m_port_path[s_index]);
        }
    }
}

} // namespace

UsbBulkDiscoveryResult discoverUsbBulkDevices() {
    UsbBulkDiscoveryResult s_result;
    const auto s_devices = UsbBulkDevice::list_devices();
    if (!s_devices) {
        s_result.m_system_error = s_devices.error();
        return s_result;
    }

    s_result.m_devices.reserve(s_devices->size());
    for (const auto& s_device : *s_devices) {
        UsbBulkDeviceInfo s_info{
            .m_vendor_id = s_device.vendor_id,
            .m_product_id = s_device.product_id,
            .m_bus_number = s_device.bus_number,
            .m_device_address = s_device.device_address,
            .m_port_path = s_device.port_path,
            .m_manufacturer = s_device.manufacturer,
            .m_product = s_device.product,
            .m_serial_number = s_device.serial_number,
            .m_display_name = {},
            .m_uri = {},
        };
        s_completeDeviceInfo(s_info);
        s_result.m_devices.push_back(std::move(s_info));
    }
    return s_result;
}

UsbBulkSelectionResult selectUsbBulkDevice(
    std::span<const UsbBulkDeviceInfo> s_devices, std::string_view s_uri) {
    UsbBulkSelectionResult s_result;
    const auto s_selector = s_parseSelector(s_uri);
    if (!s_selector) {
        s_result.m_error = UsbDiscoveryError::InvalidUri;
        return s_result;
    }

    const UsbBulkDeviceInfo* s_selected{};
    for (const auto& s_device : s_devices) {
        if (s_device.m_vendor_id != s_selector->m_vendor_id ||
            s_device.m_product_id != s_selector->m_product_id) {
            continue;
        }
        if (s_selector->m_serial_number &&
            s_device.m_serial_number != *s_selector->m_serial_number) {
            continue;
        }
        if (s_selector->m_port_path &&
            s_device.m_port_path != *s_selector->m_port_path) {
            continue;
        }
        ++s_result.m_match_count;
        if (s_selected == nullptr) s_selected = &s_device;
    }

    if (s_result.m_match_count == 0) {
        s_result.m_error = UsbDiscoveryError::DeviceNotFound;
    } else if (s_result.m_match_count > 1) {
        s_result.m_error = UsbDiscoveryError::Ambiguous;
    } else {
        s_result.m_device = *s_selected;
        s_completeDeviceInfo(*s_result.m_device);
    }
    return s_result;
}

UsbBulkSelectionResult resolveUsbBulkDevice(std::string_view s_uri) {
    auto s_discovery = discoverUsbBulkDevices();
    if (!s_discovery) {
        UsbBulkSelectionResult s_result;
        s_result.m_error = UsbDiscoveryError::EnumerationFailed;
        s_result.m_system_error = s_discovery.m_system_error;
        return s_result;
    }
    return selectUsbBulkDevice(s_discovery.m_devices, s_uri);
}

const char* usbDiscoveryErrorMessage(UsbDiscoveryError s_error) noexcept {
    switch (s_error) {
        case UsbDiscoveryError::None: return "no error";
        case UsbDiscoveryError::InvalidUri: return "invalid USB URI";
        case UsbDiscoveryError::EnumerationFailed:
            return "USB enumeration failed";
        case UsbDiscoveryError::DeviceNotFound: return "USB device not found";
        case UsbDiscoveryError::Ambiguous:
            return "USB selector matches multiple devices";
    }
    return "unknown USB discovery error";
}

} // namespace florid
