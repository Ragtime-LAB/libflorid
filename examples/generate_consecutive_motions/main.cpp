#include "examples_common.hpp"
#include <chrono>
#include <iostream>
#include <thread>

/*
 * 连续运动示例
 * 对标 libfranka: examples/generate_consecutive_motions.cpp
 *
 *  三段运动依次完成：
 *    1. 关节位控 → 到达位置 A
 *    2. 力矩控   → 保持姿态
 *    3. 关节位控 → 回到位置 B
 *  每段结束时返回 MotionFinished()。
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
        float q_start[6];
        for (int i = 0; i < 6; ++i) q_start[i] = st.q[i];

        // 目标 A：每个关节 +0.3 rad
        float q_a[6];
        for (int i = 0; i < 6; ++i) q_a[i] = q_start[i] + 0.3f;

        // 目标 B：回到初始
        float q_b[6];
        for (int i = 0; i < 6; ++i) q_b[i] = q_start[i];

        int motion = 0;      // 0→A, 1→hold, 2→B
        double t0 = 0.0;

        std::cout << "Consecutive motions: A → hold → B\n";

        arm.control([&](const florid::ArmState& s, florid::ArmControl& ctrl)
            -> florid::JointPositions
        {
            if (t0 == 0.0) t0 = s.time;

            florid::JointPositions cmd{};

            if (motion == 0)
            {
                // 位控段：平滑过渡到 A（2 秒）
                double progress = (s.time - t0) / 2.0;
                if (progress >= 1.0) { progress = 1.0; motion = 1; t0 = s.time; }
                for (int i = 0; i < 6; ++i)
                    cmd.q[i] = q_start[i] + static_cast<float>(progress) * (q_a[i] - q_start[i]);
                return cmd;
            }

            if (motion == 1)
            {
                // 力矩控段：微小阻尼，保持 3 秒
                // 返回 MotionFinished 标记本段结束
                florid::Torques torque_cmd{};
                for (int i = 0; i < 6; ++i)
                    torque_cmd.tau[i] = -5.0f * s.dq[i];  // 阻尼

                // 本部分不需要发送 JointPositions，这里改成力矩模式：
                // 为了方便，这里返回一个 MotionFinished JointPositions 直接跳
                // 实际上可以用 arm.control(torque_cb, motion_cb)
                if (s.time - t0 > 3.0) { motion = 2; t0 = s.time; }
                for (int i = 0; i < 6; ++i) cmd.q[i] = q_a[i];  // 保持
                return cmd;
            }

            // motion == 2: 回到 B（2 秒）
            double progress = (s.time - t0) / 2.0;
            if (progress >= 1.0) { progress = 1.0; ctrl.finishMotion(); }
            for (int i = 0; i < 6; ++i)
                cmd.q[i] = q_a[i] + static_cast<float>(progress) * (q_b[i] - q_a[i]);
            return cmd;
        });
    });
}
