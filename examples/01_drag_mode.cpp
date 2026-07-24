#include "florid/Arm.hpp"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

static std::atomic<bool> g_running{true};

void s_signalHandler(int) { g_running = false; }

void s_printUsage(const char* s_prog) {
    fprintf(stderr, "Usage: %s <usb_device>\n", s_prog);
    exit(1);
}

int main(int s_argc, char** s_argv) {
    if (s_argc < 2) s_printUsage(s_argv[0]);

    std::string s_uri = "usb://";
    s_uri += s_argv[1];

    signal(SIGINT, s_signalHandler);
    signal(SIGTERM, s_signalHandler);

    printf("Connecting to %s ...\n", s_uri.c_str());
    auto s_arm = florid::Arm::create(s_uri);
    if (!s_arm) {
        fprintf(stderr, "Failed to create Arm.\n");
        return 1;
    }
    printf("Connected. fw_dt=%u us\n\n", s_arm->firmwarePeriodUs());

    printf("Enabling Drag mode ...\n");
    s_arm->enable();
    printf("Drag mode enabled. Ctrl+C to stop.\n\n");

    printf(" seq  |      q0      q1      q2      q3      q4      q5  | mode | errs\n");
    printf("------|----------------------------------------------------|------|-----\n");

    int s_count = 0;
    s_arm->read([&](const florid::ArmState& s_state) {
        if (s_state.m_seq == 0) return g_running.load();

        const char* s_mn = "??";
        switch (s_state.m_mode) {
            case 0: s_mn = "Pc";  break;
            case 1: s_mn = "Drag"; break;
            case 2: s_mn = "Damp"; break;
            case 3: s_mn = "Retract"; break;
        }

        printf("%5u | %+7.3f %+7.3f %+7.3f %+7.3f %+7.3f %+7.3f | %-4s | 0x%02X\n",
               s_state.m_seq,
               s_state.m_q[0], s_state.m_q[1], s_state.m_q[2],
               s_state.m_q[3], s_state.m_q[4], s_state.m_q[5],
               s_mn, s_state.m_errors);

        s_count++;
        if (!g_running) return false;
        return true;
    });

    printf("Disabling ...\n");
    s_arm->disable();
    printf("Done. Read %d frames.\n", s_count);
    return 0;
}
