#include "examples_common.hpp"
#include <chrono>
#include <florid/protocols/cmd.hpp>
#include <iostream>
#include <thread>

/*
 * 在线切换控制模式示例
 *
 * 分段:
 *   Phase 0 → 关节位控（平滑移动到目标）
 *   Phase 1 → 笛卡尔位控（保持末端位置）
 *   Phase 2 → 关节位控（回到初始）
 */

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

    using Mode = florid::protocol::ControlMode;

    // ── 读初始状态 ────────────────────────────────────
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto st = arm.readOnce();
    float q_start[6];
    for (int i = 0; i < 6; ++i)
      q_start[i] = st.q[i];

    // 关节目标：每个 +0.2 rad
    float q_target[6];
    for (int i = 0; i < 6; ++i)
      q_target[i] = q_start[i] + 0.2f;

    // 记录初始末端位姿（用于笛卡尔段保持）
    float T_hold[16];
    std::memcpy(T_hold, st.O_T_EE, sizeof(T_hold));

    // ── 控制循环 — 手动 update+writeOnce ─────────────
    int phase = 0;
    double t_phase = 0.0;
    double t_switched = 0.0;

    std::cout << "Switch control mode demo.\n"
              << "  Phase 0: joint pos  → smooth move\n"
              << "  Phase 1: cartesian  → hold EE\n"
              << "  Phase 2: joint pos  → return\n";

    while (true) {
      auto s = arm.update();

      if (s.mode == florid::ArmMode::Fault ||
          s.mode == florid::ArmMode::EStop) {
        std::cerr << "\nFault/estop.\n";
        return;
      }

      if (t_phase == 0.0)
        t_phase = s.time;

      double elapsed = s.time - t_switched;

      // ── Phase 0: 关节位控 ──────────────────────
      if (phase == 0) {
        if (elapsed > 3.0) {
          // 切换到笛卡尔
          std::cout << "\n-> Switching to CartesianPose...\n";
          arm.switchControlMode(Mode::CartesianPose);
          t_phase = s.time;
          t_switched = s.time;
          phase = 1;
          std::memcpy(T_hold, s.O_T_EE, sizeof(T_hold));
          continue;
        }

        // 三次多项 → q_target
        double p = elapsed / 3.0;
        if (p > 1.0)
          p = 1.0;
        double s_val = 3.0 * p * p - 2.0 * p * p * p;

        florid::JointPositions cmd{};
        for (int i = 0; i < 6; ++i)
          cmd.q[i] = q_start[i] +
                     static_cast<float>(s_val) * (q_target[i] - q_start[i]);

        arm.writeOnce(cmd);
      }

      // ── Phase 1: 笛卡尔位控 ──────────────────────
      else if (phase == 1) {
        if (elapsed > 3.0) {
          // 切回关节
          std::cout << "\n-> Switching back to JointPosition...\n";
          arm.switchControlMode(Mode::JointPosition);
          t_phase = s.time;
          t_switched = s.time;
          phase = 2;
          for (int i = 0; i < 6; ++i)
            q_start[i] = s.q[i];
          continue;
        }

        // 保持末端不动
        florid::CartesianPose cmd{};
        std::memcpy(cmd.T, T_hold, sizeof(T_hold));
        arm.writeOnce(cmd);
      }

      // ── Phase 2: 回到初始 ────────────────────────
      else {
        if (elapsed > 3.0) {
          std::cout << "\n-> Done.\n";
          return;
        }

        double p = elapsed / 3.0;
        if (p > 1.0)
          p = 1.0;
        double s_val = 3.0 * p * p - 2.0 * p * p * p;

        florid::JointPositions cmd{};
        for (int i = 0; i < 6; ++i)
          cmd.q[i] = q_target[i] +
                     static_cast<float>(s_val) * (q_start[i] - q_target[i]);

        arm.writeOnce(cmd);
      }

      // ── 每一秒打一次状态 ──────────────────────────
      static int last_report = -1;
      int sec = static_cast<int>(s.time);
      if (sec != last_report) {
        last_report = sec;
        std::cout << "  t=" << static_cast<int>(elapsed) << "s"
                  << "  phase=" << phase << "  q[0]=" << s.q[0]
                  << "  EE_z=" << s.O_T_EE[14] << "         \r" << std::flush;
      }
    }
  });
}
