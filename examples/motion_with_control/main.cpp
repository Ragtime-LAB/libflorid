#include "examples_common.hpp"
#include <chrono>
#include <iostream>
#include <thread>

/*
 * 混合力矩+运动控制示例
 * 对标 libfranka: examples/motion_with_control.cpp
 *
 *  运动生成器输出关节位姿轨迹，力矩控制器叠加前馈。
 *  使用 Arm::control(torque_cb, motion_cb) 混合模式。
 */

namespace { void Usage(const char* p) { std::cerr << "Usage: " << p << " <ip>\n"; } }

int main(int argc, char* argv[])
{
    if (argc != 2) { Usage(argv[0]); return 1; }
    std::string ip;
    if (!example::parseIP(argv[1], ip)) { std::cerr << "Invalid IP\n"; return 1; }

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str());
        example::applyDefaults(arm);
        arm.setMaxFrequencyHz(1000.0);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto st = arm.readOnce();
        float q0[6];
        for (int i = 0; i < 6; ++i) q0[i] = st.q[i];

        // 目标：每个关节 +0.2 rad，5 秒完成
        float q_d[6];
        for (int i = 0; i < 6; ++i) q_d[i] = q0[i] + 0.2f;
        const double duration = 5.0;
        double t0 = 0.0;

        std::cout << "Motion + torque control — hybrid.\n";

        // ── 力矩回调：PD + 阻尼 ─────────────────────────────
        auto torque_cb = [&](const florid::RobotState& s,
                              florid::RobotControl& ctrl) -> florid::Torques
        {
            florid::Torques cmd{};
            for (int i = 0; i < 6; ++i)
                cmd.tau[i] = -10.0f * s.dq[i];  // 阻尼
            return cmd;
        };

        // ── 运动回调：三次多项轨迹 ──────────────────────────
        auto motion_cb = [&](const florid::RobotState& s,
                              florid::RobotControl& ctrl) -> florid::JointPositions
        {
            if (t0 == 0.0) t0 = s.time;
            double t = s.time - t0;
            if (t >= duration) { t = duration; ctrl.finishMotion(); }

            // 三次多项式：s(t) = 3(t/T)² - 2(t/T)³
            double s_norm = t / duration;
            double s_val  = 3.0 * s_norm * s_norm - 2.0 * s_norm * s_norm * s_norm;

            florid::JointPositions cmd{};
            for (int i = 0; i < 6; ++i)
                cmd.q[i] = q0[i] + static_cast<float>(s_val) * (q_d[i] - q0[i]);
            return cmd;
        };

        arm.control(torque_cb, motion_cb);
    });
}
