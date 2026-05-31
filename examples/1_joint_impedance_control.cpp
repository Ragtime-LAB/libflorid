#include "examples_common.hpp"
#include "florid/Model.hpp"
#include <chrono>
#include <florid/traits/WillowTraits.hpp>
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
    arm.setMaxFrequencyHz(1000.0);

    florid::Model<florid::WillowTraits> model;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    arm.readOnce();
    const float qd[6] = {0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    const float K[6] = {4.0f, 10.0f, 10.0f, 2.0f, 2.0f, 1.0f};
    const float B[6] = {0.5f, 0.8f, 0.8f, 0.2f, 0.2f, 0.1f};

    std::cout << "Joint impedance + IMU gravity comp — holding pose.\n"
              << "  (tau = gravity + Kp*(q_des-q) + Kd*(0-dq))\n";

    arm.control([&](const florid::ArmState &st,
                    florid::ArmControl &) -> florid::Torques {
      // ── IMU 重力补偿 ─────────────────────────────────
      float g_comp[6];
      model.gravity(st.q, st.base_gravity, g_comp);

      // ── 关节阻抗：重力补偿 + 位置/速度反馈 ───────────
      florid::Torques cmd{};
      for (int i = 0; i < 6; ++i) {
        const float q_err = qd[i] - st.q[i];
        const float dq_err = -st.dq[i];
        cmd.tau[i] = g_comp[i] + K[i] * q_err + B[i] * dq_err;
        cmd.kp[i] = 0.0f;
        cmd.kd[i] = 0.0f;
      }

      return cmd;
    });
  });
}
