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
#include <thread>

namespace
{
    void PrintUsage(const char* prog)
    {
        std::cerr << "Usage: " << prog << " <ip>\n  ip: arm IPv4 address\n";
    }
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

        // ── 一行配置 ──────────────────────────────────────────
        const auto& cfg = florid::kPandaConfig;
        florid::Model model(cfg);

        arm.setCollisionBehavior(cfg.collision_lower, cfg.collision_upper);
        arm.setJointImpedance(cfg.joint_impedance);
        arm.setWatchdogTimeout(florid::Duration::fromSec(cfg.watchdog_timeout_ms / 1000.0f));
        arm.setMaxFrequencyHz(1000.0);

        // ── 读初始位姿作为目标 ────────────────────────────────
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto init_state = arm.readOnce();
        float T_des[16];
        model.forwardKinematics(init_state.q, T_des);

        std::cout << "Cartesian impedance control — holding EE pose.\n";

        const float K_x[6] = {600, 600, 600, 30, 30, 30};
        const float D_x[6] = { 30,  30,  30,  2,  2,  2};

        arm.control([&](const florid::RobotState& state,
                         florid::RobotControl&) -> florid::Torques
        {
            float T_cur[16];
            model.forwardKinematics(state.q, T_cur);

            float err[6] = {};
            err[0] = T_des[12] - T_cur[12];
            err[1] = T_des[13] - T_cur[13];
            err[2] = T_des[14] - T_cur[14];
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    err[3+r] += T_des[r*4+c] * T_cur[r*4+c];
            err[3] *= 0.25f; err[4] *= 0.25f; err[5] *= 0.25f;

            float J[36];
            model.bodyJacobian(state.q, J);

            float dx[6] = {};
            for (int r = 0; r < 6; ++r)
                for (int j = 0; j < 6; ++j)
                    dx[r] += J[r * 6 + j] * state.dq[j];

            float F[6];
            for (int i = 0; i < 6; ++i)
                F[i] = K_x[i] * err[i] - D_x[i] * dx[i];

            florid::Torques cmd{};
            for (int col = 0; col < 6; ++col)
                for (int r = 0; r < 6; ++r)
                    cmd.tau[col] += J[r * 6 + col] * F[r];

            return cmd;
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
