#include "examples_common.hpp"
#include <chrono>
#include <cmath>
#include <iostream>

namespace {
    using Clock = std::chrono::steady_clock;
    constexpr double kPi = 3.14159265358979323846;
    void Usage(const char* p) { std::cerr << "Usage: " << p << " <ip> <port> <hz>\n"; }
    double ToSec(const Clock::time_point& tp) {
        return std::chrono::duration<double>(tp.time_since_epoch()).count();
    }
}

int main(int argc, char* argv[])
{
    if (argc != 4) { Usage(argv[0]); return 1; }
    std::string ip; unsigned short port; double hz;
    if (!example::parseIP(argv[1],ip)||!example::parsePort(argv[2],port)||!example::parseHz(argv[3],hz))
    { Usage(argv[0]); return 1; }

    std::cout << "Joint interpolation test @ " << hz << " Hz\n"
              << "  Cycling j1..j6, 0 -> 0.5 -> 0 rad with q+dq sine samples\n";

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str(), port);
        arm.setMaxFrequencyHz(hz);
        example::applyDefaults(arm);

        constexpr double kPeriod = 2.5;           // seconds per joint
        constexpr float  kAmplitude = 0.5f;       // 0 -> 0.5 -> 0 rad
        constexpr float  kKp = 15.0f;
        constexpr float  kKd = 0.5f;

        double t0 = 0.0;
        float base_q[6]{};

        arm.control([&](const florid::ArmState& s, florid::ArmControl&)
        {
            double t = ToSec(Clock::now());
            if (t0 == 0.0)
            {
                t0 = t;
                for (int i = 0; i < 6; ++i) base_q[i] = s.q[i];
            }
            double elapsed = t - t0;

            int joint = static_cast<int>(elapsed / kPeriod) % 6;
            double phase = std::fmod(elapsed, kPeriod);

            const double omega = 2.0 * kPi / kPeriod;
            const float val = 0.5f * kAmplitude
                * static_cast<float>(1.0 - std::cos(omega * phase));
            const float vel = 0.5f * kAmplitude
                * static_cast<float>(omega * std::sin(omega * phase));

            florid::JointPositions cmd{};
            for (int i = 0; i < 6; ++i)
            {
                cmd.q[i]  = base_q[i]; // 空闲关节保持锁存位置
                cmd.dq[i] = 0.0f;
                cmd.kp[i] = kKp;
                cmd.kd[i] = kKd;
            }

            static int last_joint = -1;
            if (joint != last_joint)
            {
                for (int i = 0; i < 6; ++i) base_q[i] = s.q[i];
                last_joint = joint;
                std::cout << "\n-> j" << (joint + 1) << ": 0 -> " << kAmplitude
                          << " -> 0"
                          << " rad  " << std::flush;
            }

            cmd.q[joint] = base_q[joint] + val;   // 活动关节在锁存位置基础上偏移
            cmd.dq[joint] = vel;

            return cmd;
        });
    });
}
