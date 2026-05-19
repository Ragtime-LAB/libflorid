#include "examples_common.hpp"
#include <chrono>
#include <iostream>
#include <thread>

/*
 * 点到点运动示例
 * 对标 libfranka: examples/joint_point_to_point_motion.cpp
 *
 *  使用 startJointPositionControl() 手动循环，
 *  三次多项式平滑地驱动各关节从起始位姿到目标位姿。
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

        // 起始位姿
        float q_start[6];
        for (int i = 0; i < 6; ++i) q_start[i] = st.q[i];

        // 目标位姿
        float q_goal[6] = {0, -0.785, 0, -2.356, 0, 1.571};

        std::cout << "Point-to-point motion.\n";

        auto control = arm.startJointPositionControl();

        bool done = false;
        double t0 = 0.0;
        const double duration = 4.0;  // 4 秒

        while (!done)
        {
            auto s = control.readOnce();

            if (t0 == 0.0) t0 = s.time;
            double t = s.time - t0;
            if (t >= duration) { t = duration; done = true; }

            // 三次多项式平滑：s(t) = 3(t/T)² - 2(t/T)³
            double s_norm = t / duration;
            double s_val  = 3.0 * s_norm * s_norm - 2.0 * s_norm * s_norm * s_norm;

            florid::JointPositions cmd{};
            for (int i = 0; i < 6; ++i)
                cmd.q[i] = q_start[i] + static_cast<float>(s_val) * (q_goal[i] - q_start[i]);

            if (done) cmd.motion_finished = true;

            control.writeOnce(cmd);

            if (static_cast<int>(t * 10) % 10 == 0)
                std::cout << "  t=" << static_cast<int>(t) << "s"
                          << "  q[1]=" << s.q[1] << "    \r" << std::flush;
        }
        std::cout << "\nDone.\n";
    });
}
