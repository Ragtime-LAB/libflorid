#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>
#include <asio.hpp>

#include <cmath>
#include <exception>
#include <iostream>
#include <string>

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

        const float F_des[6]={5,0,0,0,0,0}, Kp[6]={0.3,0.3,0.3,0.3,0.3,0.3}, Ki[6]={0.15,0.15,0.15,0.15,0.15,0.15};
        float integral[6]={};

        std::cout << "Force control — Fx=5N.\n";

        arm.control([&](const florid::RobotState& s, florid::RobotControl&) -> florid::Torques
        {
            float err[6], Fc[6];
            for(int i=0;i<6;++i){err[i]=F_des[i]-s.F_ext[i];integral[i]+=Ki[i]*err[i];Fc[i]=Kp[i]*err[i]+integral[i];}
            float J[36]; model.zeroJacobian(s.q, J);
            florid::Torques cmd{};
            for(int c=0;c<6;++c)for(int r=0;r<6;++r)cmd.tau[c]+=J[r*6+c]*Fc[r];
            return cmd;
        });
    }
    catch(const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
