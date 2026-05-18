#include <florid/Arm.hpp>
#include <florid/ArmConfig.hpp>
#include <florid/Model.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>
#include <asio.hpp>

#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>

namespace
{
    void PrintUsage(const char* p) { std::cerr << "Usage: " << p << " <ip>\n"; }
}

int main(int argc, char* argv[])
{
    if (argc != 2) { PrintUsage(argv[0]); return 1; }

    const std::string ip_text = argv[1];
    asio::error_code ec;
    asio::ip::make_address(ip_text, ec);
    if (ec) { std::cerr << "Invalid IP\n"; return 1; }

    try
    {
        constexpr uint16_t kTCP = 6041, kUDP = 6040;
        florid::detail::TcpClient tcp{ip_text, kTCP};
        if (!tcp.is_connected()) { std::cerr << "TCP refused\n"; return 1; }
        tcp.configure_session(florid::protocol::SessionMode::Joint);

        florid::detail::UdpClient udp{ip_text, kUDP};
        florid::Arm arm{udp, &tcp};

        const auto& cfg = florid::kPandaConfig;
        florid::Model model(cfg);

        arm.setCollisionBehavior(cfg.collision_lower, cfg.collision_upper);
        arm.setJointImpedance(cfg.joint_impedance);
        arm.setWatchdogTimeout(florid::Duration::fromSec(cfg.watchdog_timeout_ms / 1000.0f));
        arm.setMaxFrequencyHz(1000.0);

        const float F_des[6]  = {5.0f, 0, 0, 0, 0, 0};
        const float K_p[6]    = {0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f};
        const float K_i[6]    = {0.15f,0.15f,0.15f,0.15f,0.15f,0.15f};
        float integral[6]     = {};

        std::cout << "Force control — Fx = 5.0 N target.\n";

        arm.control([&](const florid::RobotState& state,
                         florid::RobotControl&) -> florid::Torques
        {
            float err[6], F_cmd[6];
            for (int i = 0; i < 6; ++i)
            {
                err[i] = F_des[i] - state.F_ext[i];
                integral[i] += K_i[i] * err[i];
                F_cmd[i] = K_p[i] * err[i] + integral[i];
            }

            float J[36];
            model.zeroJacobian(state.q, J);

            florid::Torques cmd{};
            for (int col = 0; col < 6; ++col)
                for (int r = 0; r < 6; ++r)
                    cmd.tau[col] += J[r * 6 + col] * F_cmd[r];

            return cmd;
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
