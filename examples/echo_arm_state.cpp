#include "florid/Arm.hpp"
#include "florid/detail/AstrialUSBTransport.hpp"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

static std::atomic<bool> g_running{true};

void s_signalHandler(int) {
    g_running = false;
}

void s_printUsage(const char* s_prog) {
    fprintf(stderr, "Usage: %s <usb_device>  (e.g. %s /dev/ttyACM0)\n", s_prog, s_prog);
    exit(1);
}

int main(int s_argc, char** s_argv) {
    if (s_argc < 2) s_printUsage(s_argv[0]);

    std::string s_uri = "usb://";
    s_uri += s_argv[1];

    signal(SIGINT, s_signalHandler);
    signal(SIGTERM, s_signalHandler);

    // ── List available USB devices ──
    printf("=== USB Devices ===\n");
    auto s_devices = florid::AstrialUSBTransport::listDevices();
    for (const auto& s_d : s_devices) {
        printf("  %-20s %04X:%04X  %s\n",
               s_d.m_port_name.c_str(), s_d.m_vendor_id, s_d.m_product_id,
               s_d.m_description.c_str());
    }
    printf("\n");

    // ── Connect ──
    printf("Connecting to %s ...\n", s_uri.c_str());
    auto s_arm = florid::Arm::create(s_uri);
    if (!s_arm) {
        fprintf(stderr, "Failed to create Arm (unknown URI scheme).\n");
        return 1;
    }

    printf("Connected. Firmware period: %u us (%.1f Hz)\n",
           s_arm->firmwarePeriodUs(),
           1e6 / s_arm->firmwarePeriodUs());

    // ── Echo state ──
    printf("\n=== Arm State Stream ===\n");
    printf(" seq  |      q0      q1      q2      q3      q4      q5  |  mode  | errs\n");
    printf("------|----------------------------------------------------|--------|------\n");

    int s_count = 0;
    s_arm->read([&](const florid::ArmState& s_state) {
        if (s_state.m_seq == 0) return g_running.load();

        printf("%5u | %+7.3f %+7.3f %+7.3f %+7.3f %+7.3f %+7.3f |",
               s_state.m_seq,
               s_state.m_q[0], s_state.m_q[1], s_state.m_q[2],
               s_state.m_q[3], s_state.m_q[4], s_state.m_q[5]);

        const char* s_mode_names[] = {"Init", "Idle", "Running", "Fault", "EStop"};
        auto s_mi = static_cast<int>(s_state.m_mode);
        const char* s_mn = (s_mi < 5) ? s_mode_names[s_mi] : "?";
        printf(" %-6s | 0x%02X\n", s_mn, s_state.m_errors);

        s_count++;
        if (s_count >= 200 || !g_running) return false;
        return true;
    });

    printf("Done. Read %d frames.\n", s_count);
    return 0;
}
