#include "examples_common.hpp"
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>
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

        florid::Model<florid::PantheraTraits> model;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto st = arm.readOnce();
        float Td[16]; model.forwardKinematics(st.q, Td);

        std::cout << "Cartesian impedance — holding EE pose.\n";
        const float Kx[6]={600,600,600,30,30,30}, Dx[6]={30,30,30,2,2,2};

        arm.control([&](const florid::RobotState& s, florid::RobotControl&)
            -> florid::Torques
        {
            float Tc[16]; model.forwardKinematics(s.q, Tc);
            float err[6]={};
            err[0]=Td[12]-Tc[12]; err[1]=Td[13]-Tc[13]; err[2]=Td[14]-Tc[14];
            for(int r=0;r<3;++r)for(int c=0;c<3;++c)err[3+r]+=Td[r*4+c]*Tc[r*4+c];
            err[3]*=.25f;err[4]*=.25f;err[5]*=.25f;

            float J[36]; model.bodyJacobian(s.q, J);
            float dx[6]={}; for(int r=0;r<6;++r)for(int j=0;j<6;++j)dx[r]+=J[r*6+j]*s.dq[j];
            float F[6]; for(int i=0;i<6;++i)F[i]=Kx[i]*err[i]-Dx[i]*dx[i];

            florid::Torques cmd{};
            for(int c=0;c<6;++c)for(int r=0;r<6;++r)cmd.tau[c]+=J[r*6+c]*F[r];
            return cmd;
        });
    });
}
