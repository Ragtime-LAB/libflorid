#include <florid/Arm.hpp>
#include <florid/core/ActiveControl.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>

#include <asio.hpp>

#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <numbers>
#include <string>
#include <thread>

namespace
{
    using Clock = std::chrono::steady_clock;

    bool ParsePort(const char* text, unsigned short& port)
    {
        try
        {
            size_t processed = 0;
            const std::string value{text};
            const unsigned long parsed = std::stoul(value, &processed, 10);
            if (processed != value.size() || parsed == 0 || parsed > 65535)
                return false;
            port = static_cast<unsigned short>(parsed);
            return true;
        }
        catch (...) { return false; }
    }

    bool ParseHz(const char* text, double& hz)
    {
        try
        {
            size_t processed = 0;
            const std::string value{text};
            hz = std::stod(value, &processed);
            return processed == value.size() && hz > 0.0;
        }
        catch (...) { return false; }
    }

    void PrintUsage(const char* prog)
    {
        std::cerr << "Usage: " << prog << " <ip> <port> <hz>\n"
                  << "  ip   : arm IPv4 address\n"
                  << "  port : UDP port for control frames\n"
                  << "  hz   : control loop frequency (≤ 1000)\n";
    }

    double ToSeconds(const Clock::time_point& now)
    {
        return std::chrono::duration<double>(now.time_since_epoch()).count();
    }
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    const std::string ip_text = argv[1];
    unsigned short port = 0;
    double hz = 0.0;

    if (!ParsePort(argv[2], port) || !ParseHz(argv[3], hz))
    {
        std::cerr << "Invalid arguments.\n";
        PrintUsage(argv[0]);
        return 1;
    }

    asio::error_code ec;
    asio::ip::make_address(ip_text, ec);
    if (ec)
    {
        std::cerr << "Invalid IP: " << ip_text << '\n';
        return 1;
    }

    std::cout << "=== Florid Active Joint Control Example ===\n"
              << "Arm: " << ip_text << ":" << port << " @ " << hz << " Hz\n";

    try
    {
        constexpr uint16_t kSessionPort = 6041;

        // ── 1. TCP 会话握手 ──────────────────────────────────
        florid::detail::TcpClient tcp{ip_text, kSessionPort};
        if (!tcp.is_connected())
        {
            std::cerr << "TCP connection refused on " << ip_text << ':' << kSessionPort << '\n';
            return 1;
        }

        if (!tcp.configure_session(florid::protocol::SessionMode::Joint))
        {
            std::cerr << "Session handshake failed\n";
            return 1;
        }
        std::cout << "TCP session established.\n";

        // ── 2. 创建 Arm 并设置保护参数 ──────────────────────
        florid::detail::UdpClient udp{ip_text, port};
        florid::Arm arm{udp, &tcp};

        const float lower_torque_limits[6] = {20, 20, 20, 20, 10, 10};
        const float upper_torque_limits[6] = {20, 20, 20, 20, 10, 10};
        const float joint_impedance[6]      = {3000, 3000, 3000, 2500, 2500, 2000};

        arm.setCollisionBehavior(lower_torque_limits, upper_torque_limits);
        arm.setJointImpedance(joint_impedance);
        arm.setWatchdogTimeout(florid::Duration::fromSec(0.2));
        arm.setMaxFrequencyHz(hz);

        std::cout << "Collision behaviour / joint impedance / watchdog configured.\n";

        // ── 3. 先读一次初始状态 ────────────────────────────
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto init_state = arm.readOnce();
        std::cout << "Initial joints: ";
        for (int i = 0; i < 6; ++i)
            std::cout << init_state.q[i] << (i < 5 ? " | " : "\n");

        // 记录启动时刻和初始位置
        const double t_start = ToSeconds(Clock::now());
        const float  q_init[6] = {
            init_state.q[0], init_state.q[1], init_state.q[2],
            init_state.q[3], init_state.q[4], init_state.q[5]
        };

        // ── 4. 使用 ActiveControl 手动循环 ─────────────────
        auto control = arm.startJointPositionControl();

        std::cout << "Entering external control loop... (Ctrl+C to stop)\n";

        auto start_time = Clock::now();
        int  iteration  = 0;

        while (true)
        {
            auto state = control.readOnce();

            if (state.mode == florid::RobotMode::Fault ||
                state.mode == florid::RobotMode::EStop)
            {
                std::cerr << "Arm entered fault/estop — stopping.\n";
                return 1;
            }

            const double t = ToSeconds(Clock::now()) - t_start;

            // 正弦关节位置轨迹
            florid::JointPositions cmd{};
            for (int i = 0; i < 6; ++i)
            {
                const double freq  = 0.2 + i * 0.1;       // Hz
                const double amp   = 0.05 + i * 0.02;      // rad
                const double phase = i * std::numbers::pi / 6.0;
                cmd.q[i] = q_init[i] + static_cast<float>(amp * std::sin(
                    2.0 * std::numbers::pi * freq * t + phase));
            }

            control.writeOnce(cmd);

            ++iteration;
            if (iteration % static_cast<int>(hz) == 0)
            {
                const auto elapsed = std::chrono::duration<double>(
                    Clock::now() - start_time).count();
                std::cout << "t=" << static_cast<int>(elapsed) << "s  "
                          << "q0=" << state.q[0] << "  "
                          << "err=" << (cmd.q[0] - state.q[0])
                          << "  fps=" << static_cast<int>(iteration / elapsed)
                          << "           \r" << std::flush;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nError: " << e.what() << '\n';
        return 1;
    }
}
