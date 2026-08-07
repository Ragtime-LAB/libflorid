#include "florid/Arm.hpp"
#include "florid/MotorRegisters.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

static std::atomic<bool> g_running{true};

void s_signalHandler(int) { g_running = false; }

void s_printUsage(const char *s_prog) {
  fprintf(stderr, "Usage: %s <usb_device>\n", s_prog);
  exit(1);
}

void s_delayMs(int s_ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(s_ms));
}

void s_printPose(florid::Arm &s_arm, const char *s_title) {
  printf("\n=== %s ===\n", s_title);
  for (int s_joint = 1; s_joint <= 7; ++s_joint) {
    auto s_pos = florid::motor::getOutputPosition(
        s_arm, static_cast<std::uint8_t>(s_joint));
    if (s_pos)
      printf("  joint %d: %.4f rad\n", s_joint, *s_pos);
    else
      printf("  joint %d: FAILED to read\n", s_joint);
    s_delayMs(20);
  }
}

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

  printf("Entering Damp (disabled) mode ...\n");
  s_arm->disable();
  s_delayMs(500);
  printf("  Arm disabled.\n");

  s_printPose(*s_arm, "Pose BEFORE zero point");

  printf("\n=== Setting zero point for all motors (1-7) ===\n");
  bool s_zero_ok = true;
  for (int s_joint = 1; s_joint <= 7; ++s_joint) {
    if (!g_running) {
      fprintf(stderr, "Interrupted.\n");
      return 1;
    }
    if (s_arm->setZeroPoint(static_cast<std::uint8_t>(s_joint))) {
      printf("  joint %d: zero point set.\n", s_joint);
    } else {
      printf("  joint %d: FAILED to set zero point.\n", s_joint);
      s_zero_ok = false;
    }
    s_delayMs(100);
  }

  s_printPose(*s_arm, "Pose AFTER zero point");

  printf("\nDone. zero_ok=%s \n", s_zero_ok ? "true" : "false");
  return (s_zero_ok) ? 0 : 1;
}
