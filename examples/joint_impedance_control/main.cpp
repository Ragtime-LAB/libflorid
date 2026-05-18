#include <florid/Arm.hpp>
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
    const std::string ip = argv[1];
    asio::error_code ec; asio::ip::make_address(ip, ec);
    if (ec) { std::cerr << "Invalid IP\n"; return 1; }

    try
    {
        constexpr uint16_t kTCP=6041, kUDP=6040;
        florid::detail::TcpClient tcp{ip, kTCP};
        if (!tcp.is_connected()) { std::cerr << "TCP refused\n"; return 1; }
        tcp.configure_session(florid::protocol::SessionMode::Joint);

        florid::detail::UdpClient udp{ip, kUDP};
        florid::Arm arm{udp, &tcp};

        arm.setCollisionBehavior(
            florid::PantheraTraits::collision_lower,
            florid::PantheraTraits::collision_upper);
        arm.setJointImpedance(florid::PantheraTraits::joint_impedance);
        arm.setWatchdogTimeout(florid::Duration(
            florid::PantheraTraits::watchdog_timeout_ms));
        arm.setMaxFrequencyHz(1000.0);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto st = arm.readOnce();
        float qd[6]; for(int i=0;i<6;++i) qd[i]=st.q[i];

        std::cout << "Joint impedance — holding pose.\n";
        const float Kp[6]={600,600,600,600,250,150}, Kd[6]={50,50,50,30,25,15};

        arm.control([&](const florid::RobotState& s, florid::RobotControl&) -> florid::Torques
        {
            florid::Torques cmd{};
            for(int i=0;i<6;++i) cmd.tau[i]=Kp[i]*(qd[i]-s.q[i])+Kd[i]*(0-s.dq[i]);
            return cmd;
        });
    }
    catch(const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
