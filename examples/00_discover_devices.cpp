#include "florid/Arm.hpp"
#include "florid/DeviceDiscovery.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {

const char* s_access(florid::DeviceAccessStatus s_status) {
    using enum florid::DeviceAccessStatus;
    switch (s_status) {
        case kNotProbed: return "not-probed";
        case kReady: return "ready";
        case kPermissionDenied: return "permission-denied";
        case kBusy: return "busy";
        case kDisconnected: return "disconnected";
        case kUnavailable: return "unavailable";
    }
    return "unknown";
}

const char* s_compatibility(florid::DeviceCompatibility s_status) {
    using enum florid::DeviceCompatibility;
    switch (s_status) {
        case kUnknown: return "unknown";
        case kCompatible: return "compatible";
        case kProtocolMismatch: return "protocol-mismatch";
        case kIdentityMismatch: return "identity-mismatch";
        case kProbeFailed: return "probe-failed";
    }
    return "unknown";
}

} // namespace

int main(int s_argc, char** s_argv) {
    bool s_probe = false;
    bool s_connect = false;
    std::uint32_t s_wait_ms = 0;
    florid::DeviceSelector s_selector;
    for (int s_index = 1; s_index < s_argc; ++s_index) {
        const std::string_view s_argument{s_argv[s_index]};
        if (s_argument == "--probe") {
            s_probe = true;
        } else if (s_argument == "--connect") {
            s_connect = true;
        } else if (s_argument == "--serial" && s_index + 1 < s_argc) {
            s_selector = florid::DeviceSelector::bySerial(s_argv[++s_index]);
        } else if (s_argument == "--name" && s_index + 1 < s_argc) {
            s_selector =
                florid::DeviceSelector::byCustomName(s_argv[++s_index]);
            s_probe = true;
        } else if (s_argument == "--wait-ms" && s_index + 1 < s_argc) {
            s_wait_ms = static_cast<std::uint32_t>(
                std::stoul(s_argv[++s_index]));
        } else {
            std::cerr << "usage: " << s_argv[0]
                      << " [--probe] [--connect] [--serial SN]"
                         " [--name NAME] [--wait-ms MS]\n";
            return 2;
        }
    }

    if (s_wait_ms != 0) {
        const auto s_waited = florid::waitForDevice(
            s_selector, std::chrono::milliseconds{s_wait_ms});
        if (!s_waited) {
            std::cerr << "wait failed: "
                      << florid::deviceDiscoveryErrorMessage(s_waited.m_error)
                      << '\n';
            return 1;
        }
        std::cout << "waited=" << s_waited.m_device->uri() << '\n';
    }

    const auto s_discovery = florid::discoverDevices({.m_probe = s_probe});
    if (!s_discovery) {
        std::cerr << "discovery failed: "
                  << florid::deviceDiscoveryErrorMessage(s_discovery.m_error);
        if (s_discovery.m_system_error) {
            std::cerr << ": " << s_discovery.m_system_error.message();
        }
        std::cerr << '\n';
        return 1;
    }

    std::cout << "devices=" << s_discovery.m_devices.size() << '\n';
    for (const auto& s_device : s_discovery.m_devices) {
        std::cout << "device uri=" << s_device.uri()
                  << " serial=" << s_device.serialNumber()
                  << " name=\"" << s_device.m_display_name << '"'
                  << " access=" << s_access(s_device.m_access)
                  << " compatibility="
                  << s_compatibility(s_device.m_compatibility);
        if (!s_device.m_error_message.empty()) {
            std::cout << " error=\"" << s_device.m_error_message << '"';
        }
        std::cout << '\n';
    }

    if (!s_connect) return 0;
    auto s_connection = florid::Arm::connect(s_selector);
    if (!s_connection) {
        std::cerr << "connect failed: "
                  << florid::armConnectionErrorMessage(s_connection.m_error);
        if (!s_connection.m_message.empty()) {
            std::cerr << ": " << s_connection.m_message;
        }
        std::cerr << '\n';
        return 1;
    }
    const auto& s_info = s_connection.m_arm->deviceInfo();
    std::cout << "connected serial=" << s_info.m_serial_number
              << " custom_name=\"" << s_info.m_custom_name << '"'
              << " board=" << s_info.m_board_name << " protocol="
              << s_info.m_protocol_version.m_major << '.'
              << s_info.m_protocol_version.m_minor << '.'
              << s_info.m_protocol_version.m_patch << '\n';
    return 0;
}
