#include "examples_common.hpp"

namespace
{
    void Usage(const char* p) { std::cerr << "Usage: " << p << " <ip>\n"; }

    const char* modeName(florid::ArmMode m)
    {
        switch (m) {
        case florid::ArmMode::Init:    return "INIT";
        case florid::ArmMode::Idle:    return "IDLE";
        case florid::ArmMode::Running: return "RUN";
        case florid::ArmMode::Fault:   return "FAULT";
        case florid::ArmMode::EStop:   return "ESTOP";
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
        std::cout << "Reading arm state...\n";

        arm.read([&](const florid::ArmState& state) -> bool
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
            return state.mode != florid::ArmMode::Fault
                && state.mode != florid::ArmMode::EStop;
        });
    });
}
