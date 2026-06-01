#include "examples_common.hpp"
#include "florid/Model.hpp"
#include <chrono>
#include <florid/traits/WillowTraits.hpp>
#include <iostream>
#include <thread>

namespace {
void Usage(const char *p) { std::cerr << "Usage: " << p << " <ip>\n"; }
} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    Usage(argv[0]);
    return 1;
  }
  std::string ip;
  if (!example::parseIP(argv[1], ip)) {
    std::cerr << "Invalid IP\n";
    return 1;
  }

  return example::runExample([&] {
    florid::Arm arm(ip.c_str());
    example::applyDefaults(arm);
    arm.setMaxFrequencyHz(500.0);

    florid::Model<florid::WillowTraits> model;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto initial = arm.update();
    for (int i = 0; i < 20 && initial.time == 0.0; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      initial = arm.update();
    }

    float qd[6]{};
    for (int i = 0; i < 6; ++i) {
      qd[i] = initial.q[i];
    }

    const float K[6] = {2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    const float B[6] = {0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    std::cout << "Joint impedance + IMU gravity comp — holding current pose.\n"
              << "  Conservative gains: tau = gravity + K*(q_des-q) - B*dq\n";

    arm.control([&](const florid::ArmState &st,
                    florid::ArmControl &) -> florid::Torques {
      // ── IMU 重力补偿 ─────────────────────────────────
      float g_comp[6];
      model.gravity(st.q, st.base_gravity, g_comp);

      // ── 关节阻抗：重力补偿 + 当前位置保持 ───────────
      florid::Torques cmd{};
      for (int i = 0; i < 6; ++i) {
        const float q_err = qd[i] - st.q[i];
        cmd.tau[i] = g_comp[i] + K[i] * q_err - B[i] * st.dq[i];
        cmd.kp[i] = 0.0f;
        cmd.kd[i] = 0.0f;
      }

      return cmd;
    });
  });
}
