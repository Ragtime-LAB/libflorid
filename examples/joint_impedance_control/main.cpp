#include <florid/Arm.hpp>
#include <florid/ArmConfig.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>
#include <asio.hpp>

#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

namespace
{
    void PrintUsage(const char* prog)
    {
        std::cerr << "Usage: " << prog << " <ip>\n"
                  << "  ip: arm IPv4 address\n";
    }
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) { PrintUsage(argv[0]); return 1; }

    const std::string ip_text = argv[1];
    asio::error_code ec;
    asio::ip::make_address(ip_text, ec);
    if (ec) { std::cerr << "Invalid IP: " << ip_text << '\n'; return 1; }

    try
    {
        constexpr uint16_t kSessionPort = 6041;
        constexpr uint16_t kUdpPort     = 6040;

        florid::detail::TcpClient tcp{ip_text, kSessionPort};
        if (!tcp.is_connected()) { std::cerr << "TCP refused\n"; return 1; }
        tcp.configure_session(florid::protocol::SessionMode::Joint);

        florid::detail::UdpClient udp{ip_text, kUdpPort};
        florid::Arm arm{udp, &tcp};

        const auto& cfg = florid::kPandaConfig;

        arm.setCollisionBehavior(cfg.collision_lower, cfg.collision_upper);
        arm.setJointImpedance(cfg.joint_impedance);
        arm.setWatchdogTimeout(florid::Duration::fromSec(cfg.watchdog_timeout_ms / 1000.0f));
        arm.setMaxFrequencyHz(1000.0);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto init_state = arm.readOnce();
        float q_d[6];
        for (int i = 0; i < 6; ++i) q_d[i] = init_state.q[i];

        std::cout << "Joint impedance control — holding initial pose.\n";

        const float K_p[6] = {600, 600, 600, 600, 250, 150};
        const float K_d[6] = { 50,  50,  50,  30,  25,  15};

        arm.control([&](const florid::RobotState& state,
                         florid::RobotControl&) -> florid::Torques
        {
            florid::Torques cmd{};
            for (int i = 0; i < 6; ++i)
            {
                float pos_err = q_d[i] - state.q[i];
                float vel_err = 0.0f - state.dq[i];
                cmd.tau[i] = K_p[i] * pos_err + K_d[i] * vel_err;
            }
            return cmd;
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
