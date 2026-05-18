#include <florid/Arm.hpp>
#include <florid/ArmConfig.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>
#include <asio.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

namespace
{
    void PrintUsage(const char* prog)
    {
        std::cerr << "Usage: " << prog << " <ip>\n"
                  << "  ip: arm IPv4 address\n";
    }

    const char* modeName(florid::RobotMode m)
    {
        switch (m)
        {
        case florid::RobotMode::Init:    return "INIT";
        case florid::RobotMode::Idle:    return "IDLE";
        case florid::RobotMode::Running: return "RUN";
        case florid::RobotMode::Fault:   return "FAULT";
        case florid::RobotMode::EStop:   return "ESTOP";
        default:                         return "?";
        }
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

        std::cout << "Reading robot state... (Ctrl+C to stop)\n";

        arm.read([&](const florid::RobotState& state) -> bool
        {
            std::cout
                << "[" << modeName(state.mode) << "] "
                << "q=[" << state.q[0] << ", " << state.q[1] << ", "
                << state.q[2] << ", " << state.q[3] << ", "
                << state.q[4] << ", " << state.q[5] << "] "
                << "errs=";
            if (state.errors == 0) std::cout << "none";
            else                   std::cout << "0x" << std::hex << state.errors << std::dec;

            float fx = state.F_ext[0], fy = state.F_ext[1], fz = state.F_ext[2];
            std::cout << " F_ext=[" << fx << ", " << fy << ", " << fz << "]";
            std::cout << "                    \r" << std::flush;

            if (state.mode == florid::RobotMode::Fault ||
                state.mode == florid::RobotMode::EStop)
            {
                std::cerr << "\nArm fault/estop — exiting.\n";
                return false;
            }
            return true;
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
