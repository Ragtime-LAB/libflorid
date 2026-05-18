#include "examples_common.hpp"
#include <chrono>
#include <thread>

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
        float qd[6];
        for (int i = 0; i < 6; ++i) qd[i] = st.q[i];

        std::cout << "Joint impedance — holding pose.\n";
        const float Kp[6] = {600, 600, 600, 600, 250, 150};
        const float Kd[6] = { 50,  50,  50,  30,  25,  15};

        arm.control([&](const florid::RobotState& st, florid::RobotControl&)
            -> florid::Torques
        {
            florid::Torques cmd{};
            for (int i = 0; i < 6; ++i)
                cmd.tau[i] = Kp[i]*(qd[i]-st.q[i]) + Kd[i]*(0-st.dq[i]);
            return cmd;
        });
    });
}
