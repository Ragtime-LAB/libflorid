#include "examples_common.hpp"

namespace
{
    void Usage(const char* p) { std::cerr << "Usage: " << p << " <ip>\n"; }

    const char* modeName(florid::RobotMode m)
    {
        switch (m) {
        case florid::RobotMode::Init:    return "INIT";
        case florid::RobotMode::Idle:    return "IDLE";
        case florid::RobotMode::Running: return "RUN";
        case florid::RobotMode::Fault:   return "FAULT";
        case florid::RobotMode::EStop:   return "ESTOP";
        default:                         return "?";
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) { Usage(argv[0]); return 1; }
    std::string ip;
    if (!example::parseIP(argv[1], ip)) { std::cerr << "Invalid IP\n"; return 1; }

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str());
        std::cout << "Reading robot state...\n";

        arm.read([&](const florid::RobotState& state) -> bool
        {
            std::cout
                << "[" << modeName(state.mode) << "] "
                << "q=[" << state.q[0] << ", " << state.q[1] << ", "
                << state.q[2] << ", " << state.q[3] << ", "
                << state.q[4] << ", " << state.q[5] << "] "
                << "errs=" << (state.errors ? "0x" : "")
                << std::hex << state.errors << std::dec
                << " F_ext=[" << state.F_ext[0] << "," << state.F_ext[1] << "," << state.F_ext[2] << "]"
                << "                    \r" << std::flush;
            return state.mode != florid::RobotMode::Fault
                && state.mode != florid::RobotMode::EStop;
        });
    });
}
