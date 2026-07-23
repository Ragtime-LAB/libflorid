#include "florid/Arm.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

static std::atomic<bool> g_running{true};

void s_signalHandler(int) { g_running = false; }

void s_printUsage(const char *s_prog) {
  fprintf(stderr, "Usage: %s <usb_device>\n", s_prog);
  exit(1);
}

constexpr float g_kp = 10.0f;
constexpr float g_kd = 0.2f;
constexpr double g_duration = 4.0; // seconds per mode

int main(int s_argc, char **s_argv) {
  if (s_argc < 2)
    s_printUsage(s_argv[0]);

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

  printf("Homing ...\n");
  s_arm->home();
  printf("Home done.\n\n");

  const char *s_mode_names[] = {"MIT", "PVT", "PosVel"};
  int s_mode_idx = 0;

  while (g_running) {
    printf("=== Mode: %s ===\n", s_mode_names[s_mode_idx]);
    auto s_start = std::chrono::steady_clock::now();

    switch (s_mode_idx) {
    case 0: // MIT
      s_arm->control([&](const florid::ArmState &,
                         florid::ArmControl &) -> florid::JointMIT {
        auto s_t = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - s_start)
                       .count();
        if (s_t > g_duration || !g_running)
          return florid::JointMIT::MotionFinished({});
        florid::JointMIT s_cmd{};
        s_cmd.m_firmware_gravity = true;
        float s_v = 0.3f * static_cast<float>(0.5 - 0.5 * std::cos(s_t * 1.5));
        if (s_v < 0.0f) s_v = 0.0f;
        s_cmd.m_q[1] = s_v; s_cmd.m_kp[1] = g_kp; s_cmd.m_kd[1] = g_kd;
        return s_cmd;
      });
      break;

    case 1: // PVT
      s_arm->control([&](const florid::ArmState &,
                         florid::ArmControl &) -> florid::JointPVT {
        auto s_t = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - s_start)
                       .count();
        if (s_t > g_duration || !g_running)
          return florid::JointPVT::MotionFinished({});
        florid::JointPVT s_cmd{};
        float s_v = 0.3f * static_cast<float>(0.5 - 0.5 * std::cos(s_t * 1.5));
        if (s_v < 0.0f) s_v = 0.0f;
        s_cmd.m_q[1] = s_v; s_cmd.m_dq_limit[1] = 1.0f;
        return s_cmd;
      });
      break;

    case 2: // PosVel
      s_arm->control([&](const florid::ArmState &,
                         florid::ArmControl &) -> florid::JointPosVel {
        auto s_t = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - s_start)
                       .count();
        if (s_t > g_duration || !g_running)
          return florid::JointPosVel::MotionFinished({});
        florid::JointPosVel s_cmd{};
        float s_v = 0.3f * static_cast<float>(0.5 - 0.5 * std::cos(s_t * 1.5));
        if (s_v < 0.0f) s_v = 0.0f;
        s_cmd.m_q[1] = s_v;
        return s_cmd;
      });
      break;
    }

    printf("  mode %s done.\n\n", s_mode_names[s_mode_idx]);
    s_mode_idx = (s_mode_idx + 1) % 3;
  }

  printf("Done.\n");
  return 0;
}
