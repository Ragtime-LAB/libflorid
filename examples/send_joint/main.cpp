#include "examples_common.hpp"
#include <chrono>
#include <cmath>
#include <numbers>

namespace {
    using Clock = std::chrono::steady_clock;
    void Usage(const char* p) { std::cerr << "Usage: " << p << " <ip> <port> <hz>\n"; }
    double ToSec(const Clock::time_point& tp) {
        return std::chrono::duration<double>(tp.time_since_epoch()).count();
    }
}

int main(int argc, char* argv[])
{
    if (argc!=4) { Usage(argv[0]); return 1; }
    std::string ip; unsigned short port; double hz;
    if (!example::parseIP(argv[1],ip)||!example::parsePort(argv[2],port)||!example::parseHz(argv[3],hz))
    { Usage(argv[0]); return 1; }
    std::cout<<"JointPositions @"<<hz<<"Hz\n";

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str(), port);
        arm.setMaxFrequencyHz(hz);
        arm.control([&](const florid::ArmState&, florid::ArmControl&) {
            double t=ToSec(Clock::now());
            florid::JointPositions cmd{};
            for(int i=0;i<6;++i){
                double f=0.2+i*.15, ph=i*std::numbers::pi/3.;
                cmd.q[i]=static_cast<float>(0.5*std::sin(2*std::numbers::pi*f*t+ph));
            }
            return cmd;
        });
    });
}
