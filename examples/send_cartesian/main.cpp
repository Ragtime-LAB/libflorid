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
    void makePose(florid::CartesianPose& p, float x, float y, float z) {
        p.T[0]=1;p.T[4]=0;p.T[8]=0;p.T[12]=x;
        p.T[1]=0;p.T[5]=1;p.T[9]=0;p.T[13]=y;
        p.T[2]=0;p.T[6]=0;p.T[10]=1;p.T[14]=z;
        p.T[3]=0;p.T[7]=0;p.T[11]=0;p.T[15]=1;
    }
}

int main(int argc, char* argv[])
{
    if (argc!=4) { Usage(argv[0]); return 1; }
    std::string ip; unsigned short port; double hz;
    if (!example::parseIP(argv[1],ip)||!example::parsePort(argv[2],port)||!example::parseHz(argv[3],hz))
    { Usage(argv[0]); return 1; }
    std::cout<<"CartesianPose @"<<hz<<"Hz\n";

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str(), port, florid::protocol::SessionMode::Point);
        arm.setMaxFrequencyHz(hz);
        constexpr float kX=300,kZ=200,kY0=-100,kY1=100; constexpr double kP=4.0;
        arm.control([&](const florid::RobotState&, florid::RobotControl&) {
            double t=ToSec(Clock::now()), ph=std::fmod(t,kP);
            double r=(ph<kP*.5)?(ph/(kP*.5)):((kP-ph)/(kP*.5));
            float y=kY0+(kY1-kY0)*static_cast<float>(r);
            florid::CartesianPose pose{}; makePose(pose,kX,y,kZ);
            return pose;
        });
    });
}
