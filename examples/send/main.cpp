#include "examples_common.hpp"
#include <chrono>
#include <cmath>
#include <numbers>
#include <random>
#include <thread>

namespace
{
    using Clock = std::chrono::steady_clock;
    void Usage(const char* p) { std::cerr<<"Usage: "<<p<<" <ip> <port> <hz> <jitter%>\n"; }
    bool parseJitter(const char* t, double& o) {
        try{size_t n=0;std::string s{t};o=std::stod(s,&n);return n==s.size()&&o>=0&&o<100;}catch(...){return false;}
    }
    double ToSec(const Clock::time_point& tp) { return std::chrono::duration<double>(tp.time_since_epoch()).count(); }
    double helix(double t) { return 20.0*t; }
    double jsine(double t) { return 100.0*std::sin(2*std::numbers::pi*1.0*t); }
    double tri(double t) {
        constexpr double P=2.0; double ph=std::fmod(t,P); double h=P*.5;
        return (ph<h)?(20.0*ph/h):(20.0*(P-ph)/h);
    }
}

int main(int argc, char* argv[])
{
    if (argc != 5) { Usage(argv[0]); return 1; }
    std::string ip; unsigned short port; double hz, jitter;
    if (!example::parseIP(argv[1],ip)||!example::parsePort(argv[2],port)||!example::parseHz(argv[3],hz)||!parseJitter(argv[4],jitter))
    { Usage(argv[0]); return 1; }
    std::cout<<"CartesianPose @"<<hz<<"Hz jitter="<<jitter<<"%\n";

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str(), port, florid::protocol::SessionMode::Point);
        arm.setMaxFrequencyHz(0.0);
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<double> js(1.-jitter/100.,1.+jitter/100.);
        auto next=Clock::now();

        arm.control([&](const florid::RobotState&, florid::RobotControl&)
        {
            next += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>((1./hz)*js(rng)));
            std::this_thread::sleep_until(next);
            double t=ToSec(Clock::now());
            florid::CartesianPose pose{};
            pose.T[0]=(float)helix(t); pose.T[1]=(float)jsine(t); pose.T[2]=(float)tri(t);
            pose.T[3]=(float)helix(.5*t); pose.T[4]=(float)jsine(.7*t); pose.T[5]=(float)tri(t+.25);
            pose.T[6]=(float)helix(.2*t); pose.T[7]=(float)jsine(1.3*t); pose.T[8]=(float)tri(t+.5);
            pose.T[9]=(float)helix(.1*t); pose.T[10]=(float)jsine(.3*t); pose.T[11]=(float)tri(t+.75);
            return pose;
        });
    });
}
