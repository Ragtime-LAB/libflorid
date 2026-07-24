#include "florid/Arm.hpp"
#include "fci_protocol/arm/constants.hpp"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

using fci::arm::MotorRegister;

static std::atomic<bool> g_running{true};

void s_signalHandler(int) { g_running = false; }

void s_printUsage(const char *s_prog) {
  fprintf(stderr, "Usage: %s <usb_device> [joint_id=1]\n  joint_id: 1-7 (1-6=arm, 7=gripper)\n", s_prog);
  exit(1);
}

void s_delayMs(int s_ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(s_ms));
}

int main(int s_argc, char **s_argv) {
  if (s_argc < 2)
    s_printUsage(s_argv[0]);

  int s_joint = 1;
  if (s_argc >= 3)
    s_joint = std::atoi(s_argv[2]);

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

  printf("Enabling (Drag mode) ...\n");
  s_arm->enable();
  s_delayMs(500);

  const auto s_rid = MotorRegister::SpeedLoopKp;

  // ── Step 1: Read original value ──
  printf("\n=== Step 1: Read SpeedLoopKp (joint %d) ===\n", s_joint);
  auto s_orig = s_arm->readMotorRegister(s_joint, s_rid);
  if (!s_orig) {
    printf("  FAILED to read\n");
    return 1;
  }
  printf("  SpeedLoopKp = %.4f\n", *s_orig);
  s_delayMs(100);

  // ── Step 2: Write new value ──
  float s_new_val = (*s_orig > 1.0f) ? (*s_orig * 0.8f) : (*s_orig * 1.2f);
  printf("\n=== Step 2: Write SpeedLoopKp = %.4f ===\n", s_new_val);
  if (!s_arm->writeMotorRegister(s_joint, s_rid, s_new_val)) {
    printf("  FAILED to write\n");
    return 1;
  }
  printf("  OK\n");
  s_delayMs(100);

  // ── Step 3: Read back to verify new value ──
  printf("\n=== Step 3: Read back (expect %.4f) ===\n", s_new_val);
  auto s_val_new = s_arm->readMotorRegister(s_joint, s_rid);
  if (s_val_new) {
    printf("  SpeedLoopKp = %.4f\n", *s_val_new);
  } else {
    printf("  FAILED to read\n");
  }
  s_delayMs(100);

  // ── Step 4: Write back original value ──
  printf("\n=== Step 4: Restore original SpeedLoopKp = %.4f ===\n", *s_orig);
  if (!s_arm->writeMotorRegister(s_joint, s_rid, *s_orig)) {
    printf("  FAILED to write\n");
    return 1;
  }
  printf("  OK\n");
  s_delayMs(100);

  // ── Step 5: Read back to verify restore ──
  printf("\n=== Step 5: Read back (expect %.4f) ===\n", *s_orig);
  auto s_val_restored = s_arm->readMotorRegister(s_joint, s_rid);
  if (s_val_restored) {
    printf("  SpeedLoopKp = %.4f\n", *s_val_restored);
  } else {
    printf("  FAILED to read\n");
  }
  s_delayMs(200);

  // ── Step 6: Disable (Damp mode) ──
  printf("\n=== Step 6: Disable (Damp mode) ===\n");
  s_arm->disable();
  s_delayMs(300);
  printf("  Arm disabled.\n");

  // ── Step 7: Store parameters ──
  printf("\n=== Step 7: Store parameters (joint %d) ===\n", s_joint);
  if (!s_arm->storeParameters(s_joint)) {
    printf("  FAILED (requires Damp mode)\n");
  } else {
    printf("  OK — parameters saved to Flash.\n");
  }

  printf("\nDone.\n");
  return 0;
}
