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

    printf("=== USB Devices ===\n");
    auto s_devices = florid::AstrialUSBTransport::listDevices();
    for (const auto& s_d : s_devices) {
        printf("  %-20s %04X:%04X  %s\n",
               s_d.m_port_name.c_str(), s_d.m_vendor_id, s_d.m_product_id,
               s_d.m_description.c_str());
    }
    printf("\n");

    printf("Connecting to %s ...\n", s_uri.c_str());
    auto s_arm = florid::Arm::create(s_uri);
    if (!s_arm) {
        fprintf(stderr, "Failed to create Arm.\n");
        return 1;
    }

    printf("Connected. Firmware period: %u us\n\n", s_arm->firmwarePeriodUs());

    printf("=== Arm Diagnostics ===\n");
    printf("  %-22s %-10s %-8s %-12s %-12s %-8s\n",
           "uptime_s", "tick", "bus_h", "tx_err", "rx_err", "mode_ms");
    printf("  %s\n", std::string(72, '-').c_str());

    int s_count = 0;
    while (g_running && s_count < 100) {
        auto s_d = s_arm->readDiagnostics();
        if (s_d.tick_count == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (s_count % 20 == 0 && s_count > 0) {
            printf("\n");
            printf("  %-22s %-10s %-8s %-12s %-12s %-8s\n",
                   "uptime_s", "tick", "bus_h", "tx_err", "rx_err", "mode_ms");
        }

        printf("  %-22u %-10u %-8u %-12u %-12u %-8u\n",
               s_d.uptime_s, s_d.tick_count,
               s_d.bus_healthy, s_d.tx_err_count, s_d.rx_err_count,
               s_d.mode_entry_ms);

        if (s_d.tick_count >= 5) {
            printf("\n  Joint diagnostics:\n");
            for (int i = 0; i < 6; ++i) {
                auto& j = s_d.joints[i];
                printf("    J%d: healthy=%u\n", i + 1, j.healthy);
            }
            auto& g = s_d.gripper;
            printf("  Gripper: healthy=%u\n", g.healthy);
            break;
        }

        ++s_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    printf("\nDone.\n");
    return 0;
}
