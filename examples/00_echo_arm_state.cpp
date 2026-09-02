#include "florid/Arm.hpp"
#include "florid/UsbDiscovery.hpp"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <thread>

#include "florid/Exceptions.hpp"

static std::atomic<bool> g_running{true};

static const char* s_modeName(std::uint32_t s_mode) {
    switch (s_mode) {
        case 0: return "PC";
        case 1: return "DRAG";
        case 2: return "DAMP";
        case 3: return "RETR";
        default: return "??";
    }
}

void s_signalHandler(int) {
    g_running = false;
}

void s_printUsage(const char* s_prog) {
    fprintf(stderr, "Usage: %s <uri>  (e.g. %s usb://2fe3:574c)\n", s_prog, s_prog);
    exit(1);
}

int main(int s_argc, char** s_argv) {//第一个参数是参数个数（包含命令行参数本身），第二个参数是参数字符串
    if (s_argc < 2) s_printUsage(s_argv[0]);//如果参数不足，打印帮助信息

    std::string s_uri = s_argv[1];

    signal(SIGINT, s_signalHandler);//注册信号处理函数，当收到SIGINT信号（Ctrl+C）时调用
    signal(SIGTERM, s_signalHandler);//注册信号处理函数，当收到SIGTERM信号（kill -9）时调用

    // ── List available USB devices ──
    printf("=== USB Devices ===\n");
    auto s_discovery = florid::discoverUsbBulkDevices();
    for (const auto& s_d : s_discovery.m_devices) {
        printf("  %-32s %-28s %s\n", s_d.m_display_name.c_str(),
               s_d.m_serial_number.c_str(), s_d.m_uri.c_str());
    }
    printf("\n");

    // ── Connect ──
    printf("Connecting to %s ...\n", s_uri.c_str());
    std::unique_ptr<florid::Arm> s_arm;
    s_arm = florid::Arm::create(s_uri);

    if (!s_arm) {
        fprintf(stderr, "Failed to create Arm (unknown URI scheme).\n");
        return 1;
    }

    printf("Connected. Firmware period: %u us (%.1f Hz)\n",
           s_arm->firmwarePeriodUs(),
           1e6 / s_arm->firmwarePeriodUs());

    // ── Device info ──
    const auto& s_info = s_arm->deviceInfo();
    printf("Device info:\n");
    printf("  Board:        %s\n", s_info.m_board_name.c_str());
    printf("  Custom name:  %s\n", s_info.m_custom_name.c_str());
    printf("  Serial:       %s\n", s_info.m_serial_number.c_str());
    printf("  FW version:   %u.%u.%u\n",
           s_info.m_firmware_version.m_major,
           s_info.m_firmware_version.m_minor,
           s_info.m_firmware_version.m_patch);
    printf("  Protocol ver: %u.%u.%u\n",
           s_info.m_protocol_version.m_major,
           s_info.m_protocol_version.m_minor,
           s_info.m_protocol_version.m_patch);
    printf("  FW type:      %u\n",
           static_cast<unsigned>(s_info.m_firmware_type));
    printf("\n");

    // ── Echo state ──
    printf("\n=== Arm State Stream ===\n");
    printf(" seq  |      q0      q1      q2      q3      q4      q5  |       x       y       z  |  mode  | errs\n");
    printf("------|----------------------------------------------------|---------------------------|--------|------\n");

    int s_count = 0;
    s_arm->read([&](const florid::ArmState& s_state) {
        if (s_state.m_seq == 0) return g_running.load();

        printf("%5u | %+7.3f %+7.3f %+7.3f %+7.3f %+7.3f %+7.3f |"
               " %+7.3f %+7.3f %+7.3f |"
               " %-6s | 0x%02X\n",
               s_state.m_seq,
               s_state.m_q[0], s_state.m_q[1], s_state.m_q[2],
               s_state.m_q[3], s_state.m_q[4], s_state.m_q[5],
               s_state.m_O_T_EE[12], s_state.m_O_T_EE[13], s_state.m_O_T_EE[14],
               s_modeName(s_state.m_mode), s_state.m_errors);

        s_count++;
        if (s_count >= 200 || !g_running) return false;
        return true;
    });

    printf("Done. Read %d frames.\n", s_count);
    return 0;
}
