#include "examples_common.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

/*
 * 关节速度运动示例
 * 对标 libfranka: examples/generate_joint_velocity_motion.cpp
 *
 *  六关节各自以不同正弦频率振荡速度，臂端插值到 1kHz 位置控制。
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
        double t0 = st.time;

        std::cout << "Joint velocity motion — sinusoidal.\n";

        arm.control([&](const florid::ArmState& s, florid::ArmControl&)
            -> florid::JointVelocities
        {
            double t = s.time - t0;
            florid::JointVelocities cmd{};
            for (int i = 0; i < 6; ++i)
            {
                double freq  = 0.1 + i * 0.1;
                double amp   = 0.2 + i * 0.05;
                cmd.dq[i] = static_cast<float>(amp * std::sin(2 * 3.141592653589793 * freq * t));
            }
            return cmd;
        });
    });
}
