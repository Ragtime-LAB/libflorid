#include "examples_common.hpp"
#include <chrono>
#include <cmath>
#include <iostream>

namespace {
    using Clock = std::chrono::steady_clock;
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
              << "  Cycling j1..j6, 3s each, 0 ↔ 0.4 rad\n";

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str(), port);
        arm.setMaxFrequencyHz(hz);
        example::applyDefaults(arm);

        constexpr double kPeriod = 3.0;          // 每个关节 3 秒
        constexpr float  kAmplitude = 0.4f;       // 0 → 0.4 → 0 rad
        constexpr float  kKp = 20.0f;
        constexpr float  kKd = 0.4f;

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

            float val;
            if (phase < kPeriod * 0.5)
                val = kAmplitude * static_cast<float>(phase / (kPeriod * 0.5));
            else
                val = kAmplitude * static_cast<float>((kPeriod - phase) / (kPeriod * 0.5));

            florid::JointPositions cmd{};
            for (int i = 0; i < 6; ++i)
            {
                cmd.q[i]  = base_q[i]; // 空闲关节保持锁存位置
                cmd.kp[i] = kKp;
                cmd.kd[i] = kKd;
            }

            static int last_joint = -1;
            if (joint != last_joint)
            {
                for (int i = 0; i < 6; ++i) base_q[i] = s.q[i];
                last_joint = joint;
                std::cout << "\n-> j" << (joint + 1) << ": 0 ↔ " << kAmplitude
                          << " rad  " << std::flush;
            }

            cmd.q[joint] = base_q[joint] + val;   // 活动关节在锁存位置基础上偏移

            return cmd;
        });
    });
}
