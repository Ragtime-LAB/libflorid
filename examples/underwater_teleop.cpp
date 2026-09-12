#include <florid/UnderwaterTeleop.hpp>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string_view>
#include <vector>

namespace {
bool version(std::string_view s_text, florid::Version& s_version) {
    std::uint32_t* s_fields[]{&s_version.m_major, &s_version.m_minor, &s_version.m_patch};
    for (unsigned s_i = 0; s_i < 3; ++s_i) {
        const auto s_end = s_text.find('.');
        const auto s_part = s_text.substr(0, s_end);
        const auto s_parse = std::from_chars(s_part.data(), s_part.data() + s_part.size(), *s_fields[s_i]);
        if (s_part.empty() || s_parse.ec != std::errc{} || s_parse.ptr != s_part.data() + s_part.size() ||
            ((s_i == 2) != (s_end == std::string_view::npos))) return false;
        if (s_i < 2) s_text.remove_prefix(s_end + 1);
    }
    return true;
}
int print(const florid::UnderwaterRpcResult& s_result) {
    std::printf("rpc_error=%d device_status=%d accepted=%d\n", int(s_result.m_error),
                s_result.m_device_status, bool(s_result));
    if (s_result.m_info) {
        const auto& s_info = *s_result.m_info;
        std::printf("mode=%d uptime=%u enabling=0x%x gravity=0x%x upgrading=%d\n"
            "master available=0x%x enabled=0x%x disable_pending=0x%x\n"
            "slave  available=0x%x enabled=0x%x disable_pending=0x%x\n%s\n",
            int(s_info.m_mode), s_info.m_uptime_ms, s_info.m_enabling_arms, s_info.m_gravity_arms, s_info.m_upgrading,
            s_info.m_master_available_mask, s_info.m_master_enabled_mask, s_info.m_master_disable_pending_mask,
            s_info.m_slave_available_mask, s_info.m_slave_enabled_mask, s_info.m_slave_disable_pending_mask,
            s_info.m_reason.c_str());
    }
    return s_result ? 0 : 1;
}
}

int main(int argc, char** argv) {
    const std::string_view s_action = argc >= 3 ? argv[2] : "";
    florid::Version s_version{};
    const bool s_query = argc == 3 && (s_action == "status" || s_action == "enable" || s_action == "disable" ||
        s_action == "teleop" || s_action == "boot" || s_action == "reboot");
    if (!s_query && !(argc == 4 && s_action == "upload") &&
        !(argc == 5 && s_action == "update" && version(argv[4], s_version))) {
        std::printf("Usage: %s USB_URI status|enable|disable|teleop|boot|reboot\n"
            "       %s USB_URI upload zephyr.signed.bin\n"
            "       %s USB_URI update zephyr.signed.bin MAJOR.MINOR.PATCH\n"
            "Closing this program leaves the robot in its current mode. Use disable explicitly.\n", argv[0], argv[0], argv[0]);
        return argc == 2 && std::string_view(argv[1]) == "--help" ? 0 : 2;
    }
    auto s_connection = florid::UnderwaterTeleop::connect(std::string(argv[1]));
    if (!s_connection) {
        std::fprintf(stderr, "connect: %s %s\n", s_connection.m_message.c_str(), s_connection.m_system_error.message().c_str());
        return 1;
    }
    auto& s_device = *s_connection.m_device;
    if (s_action == "status") return print(s_device.status());
    if (s_action == "enable") return print(s_device.enable());
    if (s_action == "disable") return print(s_device.disable());
    if (s_action == "teleop") return print(s_device.enterTeleop());
    if (s_action == "boot") {
        const auto s_boot = s_device.bootStatus();
        std::printf("error=%d device_status=%d active=%d confirmed=%d pending=%d bytes=%llu\n",
            int(s_boot.m_error), s_boot.m_device_status, s_boot.m_active_slot, s_boot.m_confirmed_slot,
            s_boot.m_pending_slot, static_cast<unsigned long long>(s_boot.m_bytes_written));
        return s_boot.m_error == florid::FirmwareUpdateError::kNone ? 0 : 1;
    }
    if (s_action == "reboot") {
        const auto s_reboot = s_device.reboot();
        std::printf("error=%d device_status=%d accepted=%d departed=%d\n", int(s_reboot.m_error),
            s_reboot.m_device_status, s_reboot.m_request_accepted, s_reboot.m_departure_observed);
        return s_reboot.m_error == florid::FirmwareUpdateError::kNone ? 0 : 1;
    }
    std::ifstream s_file(argv[3], std::ios::binary);
    if (!s_file) { std::fprintf(stderr, "cannot open %s\n", argv[3]); return 1; }
    const std::vector<std::uint8_t> s_image{std::istreambuf_iterator<char>(s_file), {}};
    if (s_action == "upload") {
        const auto s_upload = s_device.uploadFirmware(s_image);
        std::printf("upload state=%d error=%d status=%d bytes=%llu\n", int(s_upload.m_state), int(s_upload.m_error),
            s_upload.m_device_status, static_cast<unsigned long long>(s_upload.m_acknowledged_bytes));
        return s_upload.m_state == florid::FirmwareUploadState::kCompleted ? 0 : 1;
    }
    const auto s_updated = s_device.updateFirmware(s_image, s_version);
    std::printf("upload_error=%d device_status=%d install_error=%d target_confirmed=%d\n", int(s_updated.m_upload.m_error),
        s_updated.m_upload.m_device_status, s_updated.m_install ? int(s_updated.m_install->m_error) : -1, bool(s_updated));
    return s_updated ? 0 : 1;
}
