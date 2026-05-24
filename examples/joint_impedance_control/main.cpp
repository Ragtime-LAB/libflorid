#include "examples_common.hpp"
#include <chrono>
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>
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

    florid::Model<florid::PantheraTraits> model;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto st = arm.readOnce();
    float qd[6];
    for (int i = 0; i < 6; ++i)
      qd[i] = st.q[i];

    std::cout
        << "Joint impedance + IMU gravity comp — holding pose.\n"
        << "  (applies kp/kd from each callback, sent per-frame — no TCP/UDP race)\n";

    const float Kp[6] = {30, 30, 30, 15, 10, 5};
    const float Kd[6] = {3, 3, 3, 2, 1, 0.5};

    arm.control([&](const florid::ArmState &st,
                    florid::ArmControl &) -> florid::Torques {
      // ── IMU 重力补偿 ─────────────────────────────────
      //   base_gravity 来自臂端 IMU，自动适应桌装/墙装/倒装
      float g_comp[6];
      model.gravity(st.q, st.base_gravity, g_comp);

      // ── PD 控制 ──────────────────────────────────────
      florid::Torques cmd{};
      for (int i = 0; i < 6; ++i)
        cmd.tau[i] =
            Kp[i] * (qd[i] - st.q[i]) + Kd[i] * (0 - st.dq[i]) + g_comp[i];

      return cmd;
    });
  });
}
