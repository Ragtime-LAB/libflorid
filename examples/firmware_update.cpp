#include <florid/FirmwareUpdater.hpp>
#include <chrono>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace florid;

namespace {
void print_boot(const BootStatus &s_boot) {
    std::printf("boot: error=%d status=%d state=%d active=%d confirmed=%d pending=%d chunk=%u bytes=%llu\n"
                "slots: primary=0x%08x/%u secondary=0x%08x/%u\n",
                static_cast<int>(s_boot.m_error), s_boot.m_device_status, s_boot.m_boot_state,
                s_boot.m_active_slot, s_boot.m_confirmed_slot, s_boot.m_pending_slot,
                s_boot.m_max_chunk_size, static_cast<unsigned long long>(s_boot.m_bytes_written),
                s_boot.m_slot0_address, s_boot.m_slot0_size, s_boot.m_slot1_address, s_boot.m_slot1_size);
}
int reboot(FirmwareUpdater &s_updater, FirmwareRebootMode s_mode = FirmwareRebootMode::kTryBoot) {
    const auto s_result = s_updater.reboot(s_mode);
    if (s_result.m_error != FirmwareUpdateError::kNone) {
        std::fprintf(stderr, "Reboot outcome uncertain/rejected: error=%d status=%d accepted=%d departed=%d. Query again; do not blindly retry.\n",
                     static_cast<int>(s_result.m_error), s_result.m_device_status,
                     s_result.m_request_accepted, s_result.m_departure_observed);
        return 1;
    }
    std::puts("Reboot accepted and departure observed. Installation is NOT yet verified; check confirmed slot/version.");
    return 0;
}
bool parse_version(std::string_view s_text, Version& s_version) {
    std::uint32_t* s_parts[]{&s_version.m_major, &s_version.m_minor, &s_version.m_patch};
    for (unsigned s_i = 0; s_i < 3; ++s_i) {
        const auto s_end = s_text.find('.');
        const auto s_part = s_text.substr(0, s_end);
        const auto s_result = std::from_chars(s_part.data(), s_part.data() + s_part.size(), *s_parts[s_i]);
        if (s_part.empty() || s_result.ec != std::errc{} || s_result.ptr != s_part.data() + s_part.size() ||
            ((s_i == 2) != (s_end == std::string_view::npos))) return false;
        if (s_i < 2) s_text.remove_prefix(s_end + 1);
    }
    return true;
}
int install(FirmwareUpdater& s_updater, const Version& s_expected) {
    const auto s_result = s_updater.rebootAndWait(s_expected);
    if (s_result.m_boot_status) print_boot(*s_result.m_boot_status);
    if (s_result) { std::puts("Target FCI version is running and primary is confirmed."); return 0; }
    if (s_result.m_state == FirmwareInstallState::kPreviousFirmware)
        std::fputs("Previous firmware is still running; update was not installed (or reverted).\n", stderr);
    else std::fprintf(stderr, "Installation not verified: error=%d reboot_error=%d. Do not blindly retry.\n",
                      static_cast<int>(s_result.m_error), static_cast<int>(s_result.m_reboot.m_error));
    return 1;
}
}

int main(int argc, char **argv) {
    const bool s_wait_only = argc == 4 && std::string_view(argv[2]) == "--reboot-and-wait";
    const bool s_install = argc == 5 && std::string_view(argv[3]) == "--install";
    const bool s_reboot = argc == 4 && std::string_view(argv[3]) == "--reboot" &&
                          !std::string_view(argv[2]).starts_with("--");
    Version s_expected{};
    if ((argc != 3 && !s_wait_only && !s_install && !s_reboot) ||
        (s_wait_only && !parse_version(argv[3], s_expected)) ||
        (s_install && (!parse_version(argv[4], s_expected) || std::string_view(argv[2]).starts_with("--")))) {
        std::fprintf(stderr, "Usage: %s USB_URI --status|--reboot-only|--reboot-normal\n"
            "       %s USB_URI --reboot-and-wait MAJOR.MINOR.PATCH\n"
            "       %s USB_URI zephyr.signed.bin [--reboot | --install MAJOR.MINOR.PATCH]\n", argv[0], argv[0], argv[0]);
        return argc == 2 && std::string_view(argv[1]) == "--help" ? 0 : 2;
    }
    auto s_connection = FirmwareUpdater::connect(std::string(argv[1]));
    if (!s_connection) {
        std::fprintf(stderr, "Upgrade connection failed: error=%d system=%d %s\n",
            static_cast<int>(s_connection.m_error), s_connection.m_system_error.value(), s_connection.m_error_message.c_str());
        return 1;
    }
    auto s_updater = std::move(s_connection.m_updater);
    const auto& s_info = *s_connection.m_device->m_device_info;
    std::printf("device: serial=%s board=%s FCI firmware=%u.%u.%u\n", s_info.m_serial_number.c_str(),
        s_info.m_board_name.c_str(), s_info.m_firmware_version.m_major,
        s_info.m_firmware_version.m_minor, s_info.m_firmware_version.m_patch);
    if (s_install && s_info.m_firmware_version.m_major == s_expected.m_major &&
        s_info.m_firmware_version.m_minor == s_expected.m_minor && s_info.m_firmware_version.m_patch == s_expected.m_patch) {
        std::fputs("Cannot verify a same-version replacement with FCI; use upload/--reboot for an explicit same-version reflash.\n", stderr);
        return 2;
    }
    const auto s_boot = s_updater->bootStatus();
    print_boot(s_boot);
    if (s_boot.m_error != FirmwareUpdateError::kNone) return 1;
    if (std::string_view(argv[2]) == "--status") return 0;
    if (std::string_view(argv[2]) == "--reboot-only") return reboot(*s_updater);
    if (std::string_view(argv[2]) == "--reboot-normal") return reboot(*s_updater, FirmwareRebootMode::kNormal);
    if (s_wait_only) return install(*s_updater, s_expected);
    if (!s_boot.m_max_chunk_size || !s_boot.m_slot1_size) {
        std::fputs("Device has no MCUboot storage sink; install the sysbuild bootloader/application through SWD first.\n", stderr);
        return 1;
    }
    std::ifstream s_file(argv[2], std::ios::binary | std::ios::ate);
    const auto s_size = s_file ? s_file.tellg() : std::streampos(-1);
    if (s_size < std::streampos(32) || s_size > std::streampos(s_boot.m_slot1_size)) {
        std::fputs("Invalid image file or file larger than secondary slot.\n", stderr); return 1;
    }
    std::vector<std::uint8_t> s_image(static_cast<std::size_t>(s_size));
    s_file.seekg(0);
    if (!s_file.read(reinterpret_cast<char *>(s_image.data()), static_cast<std::streamsize>(s_image.size())) ||
        s_image[0] != 0x3d || s_image[1] != 0xb8 || s_image[2] != 0xf3 || s_image[3] != 0x96) {
        std::fputs("Expected an unpadded MCUboot zephyr.signed.bin, not ELF/HEX/raw/merged flash image.\n", stderr); return 1;
    }
    std::puts("Uploading: keep the arm mechanically supported and do not send motion commands.");
    const auto s_started = s_updater->startUpload(s_image); // SDK internal-flash timing defaults
    if (s_started != FirmwareUpdateError::kNone) {
        std::fprintf(stderr, "Upload admission failed: %d\n", static_cast<int>(s_started)); return 1;
    }
    auto s_report_at = std::chrono::steady_clock::now();
    while (s_updater->progress().active()) {
        if (std::chrono::steady_clock::now() >= s_report_at) {
            const auto s_progress = s_updater->progress();
            std::printf("uploaded %llu/%llu bytes, retries=%u\n",
                        static_cast<unsigned long long>(s_progress.m_acknowledged_bytes),
                        static_cast<unsigned long long>(s_progress.m_total_bytes), s_progress.m_retries);
            s_report_at = std::chrono::steady_clock::now() + 1s;
        }
        std::this_thread::sleep_for(10ms);
    }
    const auto s_done = s_updater->progress();
    if (s_done.m_state != FirmwareUploadState::kCompleted) {
        std::fprintf(stderr, "Upload failed: error=%d device_status=%d\n", static_cast<int>(s_done.m_error), s_done.m_device_status);
        return 1;
    }
    print_boot(s_updater->bootStatus());
    std::puts("Stored and pending test boot. MCUboot authenticates the signature on reboot; upload completion is not installation confirmation.");
    return s_install ? install(*s_updater, s_expected) : s_reboot ? reboot(*s_updater) : 0;
}
