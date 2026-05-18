#include "examples_common.hpp"
#include <florid/core/ActiveControl.hpp>
#include <chrono>
#include <cmath>
#include <numbers>
#include <thread>

namespace
{
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
    if (!example::parseIP(argv[1],ip)) { std::cerr<<"Bad IP\n"; return 1; }
    if (!example::parsePort(argv[2],port)) { std::cerr<<"Bad port\n"; return 1; }
    if (!example::parseHz(argv[3],hz)) { std::cerr<<"Bad Hz\n"; return 1; }

    return example::runExample([&]
    {
        florid::Arm arm(ip.c_str(), port);
        example::applyDefaults(arm);
        arm.setMaxFrequencyHz(hz);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto st = arm.readOnce();
        float qi[6]; for(int i=0;i<6;++i)qi[i]=st.q[i];

        auto control = arm.startJointPositionControl();
        std::cout << "Active joint control.\n";
        double t0=ToSec(Clock::now()); auto start=Clock::now(); int iter=0;

        while(true) {
            st=control.readOnce();
            if(st.mode==florid::RobotMode::Fault||st.mode==florid::RobotMode::EStop){std::cerr<<"\nFault\n";return;}
            double t=ToSec(Clock::now())-t0;
            florid::JointPositions cmd{};
            for(int i=0;i<6;++i){
                double f=0.2+i*.1,a=0.05+i*.02,ph=i*std::numbers::pi/6.;
                cmd.q[i]=qi[i]+static_cast<float>(a*std::sin(2*std::numbers::pi*f*t+ph));
            }
            control.writeOnce(cmd);
            ++iter;
            if(iter%static_cast<int>(hz)==0){
                double el=std::chrono::duration<double>(Clock::now()-start).count();
                std::cout<<"t="<<static_cast<int>(el)<<"s q0="<<st.q[0]<<" fps="<<static_cast<int>(iter/el)<<"  \r"<<std::flush;
            }
        }
    });
}
