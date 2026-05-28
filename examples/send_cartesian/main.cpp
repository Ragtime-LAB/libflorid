#include "examples_common.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace {
    void Usage(const char* p) { std::cerr << "Usage: " << p << " <ip> <port> <hz>\n"; }
}

int main(int argc, char* argv[])
{
    if (argc != 4) { Usage(argv[0]); return 1; }
    std::string ip; unsigned short port; double hz;
    if (!example::parseIP(argv[1],ip)||!example::parsePort(argv[2],port)||!example::parseHz(argv[3],hz))
    { Usage(argv[0]); return 1; }

    std::cout << "Cartesian hold test @ " << hz << " Hz\n"
              << "  Waiting for valid telemetry...\n";

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str(), port, florid::protocol::SessionMode::Point);
        arm.setMaxFrequencyHz(hz);

        // 等臂端推来有效状态再抓 T_hold
        float T_hold[16];
        while (true)
        {
            auto s = arm.readOnce();
            if (s.O_T_EE[15] == 1.0f)
            {
                std::memcpy(T_hold, s.O_T_EE, sizeof(T_hold));
                std::cout << "  EE position: ("
                          << T_hold[12] << ", " << T_hold[13] << ", " << T_hold[14]
                          << ")\n  Holding...\n";
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        arm.control([&](const florid::ArmState&, florid::ArmControl&)
        {
            florid::CartesianPose cmd{};
            std::memcpy(cmd.T, T_hold, sizeof(cmd.T));
            return cmd;
        });
    });
}
