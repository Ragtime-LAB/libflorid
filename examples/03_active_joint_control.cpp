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

  // ── Active control: manual polling loop ──
  printf("Starting active joint hold (kp=25, kd=0.4, firmware gravity)\n");

  auto s_ctrl = s_arm->startJointMITControl();

  bool s_initialized = false;
  float s_q_des[6]{};
  int s_frame = 0;

  while (g_running) {
    auto s_state = s_ctrl->readOnce();
    if (s_state.m_seq == 0)
      continue;

    if (!s_initialized) {
      for (int s_i = 0; s_i < 6; ++s_i)
        s_q_des[s_i] = s_state.m_q[s_i];
      s_initialized = true;
    }

    florid::JointMIT s_cmd;
    s_cmd.m_firmware_gravity = true;

    for (int s_i = 0; s_i < 6; ++s_i) {
      s_cmd.m_q[s_i] = s_q_des[s_i];
      s_cmd.m_dq[s_i] = 0.0f;
      s_cmd.m_tau[s_i] = 0.0f;
      s_cmd.m_kp[s_i] = 25.0f;
      s_cmd.m_kd[s_i] = 0.4f;
    }

    s_ctrl->writeOnce(s_cmd);

    s_frame++;
    if (s_frame % 500 == 0) {
      printf("  [%d] q0=%.3f q1=%.3f q2=%.3f\n", s_frame, s_state.m_q[0],
             s_state.m_q[1], s_state.m_q[2]);
    }
  }

  printf("Control loop ended. Sent %d frames.\n", s_frame);
  return 0;
}
