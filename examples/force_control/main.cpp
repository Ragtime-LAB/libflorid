#include "examples_common.hpp"
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>

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

        florid::Model<florid::PantheraTraits> model;
        const float Fd[6]={5,0,0,0,0,0},Kp[6]={.3,.3,.3,.3,.3,.3},Ki[6]={.15,.15,.15,.15,.15,.15};
        float integral[6]={};

        std::cout << "Force control — Fx=5N.\n";
        arm.control([&](const florid::RobotState& s, florid::RobotControl&) -> florid::Torques
        {
            float err[6],Fc[6];
            for(int i=0;i<6;++i){err[i]=Fd[i]-s.F_ext[i];integral[i]+=Ki[i]*err[i];Fc[i]=Kp[i]*err[i]+integral[i];}
            float J[36]; model.zeroJacobian(s.q, J);
            florid::Torques cmd{};
            for(int c=0;c<6;++c)for(int r=0;r<6;++r)cmd.tau[c]+=J[r*6+c]*Fc[r];
            return cmd;
        });
    });
}
