#include "florid/Arm.hpp"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

static std::atomic<bool> g_running{true};

void s_signalHandler(int) { g_running = false; }

void s_printUsage(const char *s_prog) {
  fprintf(stderr, "Usage: %s <uri>\n", s_prog);
  exit(1);
}

int main(int s_argc, char **s_argv) {
  if (s_argc < 2)
    s_printUsage(s_argv[0]);

  std::string s_uri = s_argv[1];

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

  // ── Gripper setup ──
  auto &s_gripper = s_arm->gripper();
  auto s_ctrl = s_gripper.startJointMITControl();

  // Read initial position
  float s_current = 0.0f;
  for (int s_i = 0; s_i < 50; ++s_i) {
    auto s_state = s_ctrl->readOnce();
    if (s_state.m_seq != 0) {
      s_current = s_state.m_gripper_q;
      break;
    }
  }
  printf("Initial gripper position: %.4f\n\n", s_current);

  // ── Move loop: open ↔ close ──
  float s_target = s_current;
  float s_open   = s_current - 0.5f;
  float s_close  = s_current;
  int s_phase_frames = 0;
  int s_frame = 0;
  bool s_is_open = false;

  printf("Gripper MIT control (kp=20, kd=0.5). Cycling open<->close.\n");

  while (g_running) {
    auto s_state = s_ctrl->readOnce();
    if (s_state.m_seq == 0)
      continue;

    s_phase_frames++;

    if (s_phase_frames > 1000) {
      s_phase_frames = 0;
      s_is_open = !s_is_open;
      s_target = s_is_open ? s_open : s_close;
      printf("  -> target=%.4f\n", s_target);
    }

    florid::JointMIT s_cmd;
    s_cmd.m_q[0] = s_target;
    s_cmd.m_dq[0] = 0.0f;
    s_cmd.m_tau[0] = 0.0f;
    s_cmd.m_kp[0] = 20.0f;
    s_cmd.m_kd[0] = 0.5f;

    s_ctrl->writeOnce(s_cmd);

    if (++s_frame % 500 == 0) {
      printf("  [%d] gripper_q=%.4f  target=%.4f\n", s_frame,
             s_state.m_gripper_q, s_target);
    }
  }

  printf("Done. Sent %d frames.\n", s_frame);
  return 0;
}
