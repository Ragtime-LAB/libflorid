#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>
#include <asio.hpp>

#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

namespace { void PrintUsage(const char* p) { std::cerr << "Usage: " << p << " <ip>\n"; } }

int main(int argc, char* argv[])
{
    if (argc != 2) { PrintUsage(argv[0]); return 1; }
    const std::string ip_text = argv[1];
    asio::error_code ec; asio::ip::make_address(ip_text, ec);
    if (ec) { std::cerr << "Invalid IP\n"; return 1; }

    try
    {
        constexpr uint16_t kTCP=6041, kUDP=6040;
        florid::detail::TcpClient tcp{ip_text, kTCP};
        if (!tcp.is_connected()) { std::cerr << "TCP refused\n"; return 1; }
        tcp.configure_session(florid::protocol::SessionMode::Joint);

        florid::detail::UdpClient udp{ip_text, kUDP};
        florid::Arm arm{udp, &tcp};

        florid::Model<florid::PantheraTraits> model;

        arm.setCollisionBehavior(
            florid::PantheraTraits::collision_lower,
            florid::PantheraTraits::collision_upper);
        arm.setJointImpedance(florid::PantheraTraits::joint_impedance);
        arm.setWatchdogTimeout(florid::Duration(
            florid::PantheraTraits::watchdog_timeout_ms));
        arm.setMaxFrequencyHz(1000.0);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto init_state = arm.readOnce();
        float T_des[16];
        model.forwardKinematics(init_state.q, T_des);

        std::cout << "Cartesian impedance control.\n";

        const float K_x[6] = {600,600,600,30,30,30};
        const float D_x[6] = { 30, 30, 30, 2, 2, 2};

        arm.control([&](const florid::RobotState& s, florid::RobotControl&) -> florid::Torques
        {
            float Tc[16]; model.forwardKinematics(s.q, Tc);
            float err[6]={};
            err[0]=T_des[12]-Tc[12]; err[1]=T_des[13]-Tc[13]; err[2]=T_des[14]-Tc[14];
            for(int r=0;r<3;++r)for(int c=0;c<3;++c)err[3+r]+=T_des[r*4+c]*Tc[r*4+c];
            err[3]*=0.25f;err[4]*=0.25f;err[5]*=0.25f;

            float J[36]; model.bodyJacobian(s.q, J);
            float dx[6]={};
            for(int r=0;r<6;++r)for(int j=0;j<6;++j)dx[r]+=J[r*6+j]*s.dq[j];

            float F[6];
            for(int i=0;i<6;++i)F[i]=K_x[i]*err[i]-D_x[i]*dx[i];

            florid::Torques cmd{};
            for(int c=0;c<6;++c)for(int r=0;r<6;++r)cmd.tau[c]+=J[r*6+c]*F[r];
            return cmd;
        });
    }
    catch(const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
