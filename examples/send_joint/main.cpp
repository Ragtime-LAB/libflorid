#include <florid/Arm.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>

#include <florid/protocols/cmd.hpp>

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
            {
                return false;
            }
            port = static_cast<unsigned short>(parsed);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool ParseFrequency(const char* text, double& hz)
    {
        try
        {
            size_t processed = 0;
            const std::string value{text};
            hz = std::stod(value, &processed);
            return processed == value.size() && hz > 0.0;
        }
        catch (...)
        {
            return false;
        }
    }

    void PrintUsage(const char* program)
    {
        std::cerr << "Usage: " << program << " <ip> <port> <hz>\n"
            << "  ip   : target IPv4/IPv6 address\n"
            << "  port : UDP port in range [1, 65535]\n"
            << "  hz   : base send frequency, must be > 0\n";
    }

    double ToSeconds(const Clock::time_point& now)
    {
        const auto elapsed = std::chrono::duration<double>(now.time_since_epoch());
        return elapsed.count();
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

    if (!ParsePort(argv[2], port))
    {
        std::cerr << "Invalid port: " << argv[2] << '\n';
        PrintUsage(argv[0]);
        return 1;
    }

    if (!ParseFrequency(argv[3], hz))
    {
        std::cerr << "Invalid frequency: " << argv[3] << '\n';
        PrintUsage(argv[0]);
        return 1;
    }

    asio::error_code ec;
    const auto address = asio::ip::make_address(ip_text, ec);
    if (ec)
    {
        std::cerr << "Invalid IP address: " << ip_text << " (" << ec.message() << ")\n";
        return 1;
    }

    std::cout << "Sending JointPositions to " << address.to_string() << ':' << port << " at " << hz << " Hz\n";

    try
    {
        constexpr uint16_t kSessionPort = 6041;

        florid::detail::TcpClient tcp{ip_text, kSessionPort};
        if (!tcp.is_connected())
        {
            std::cerr << "Failed to connect TCP session to " << ip_text << ':' << kSessionPort << '\n';
            return 1;
        }

        if (!tcp.configure_session(florid::protocol::SessionMode::Joint))
        {
            std::cerr << "TCP session handshake failed\n";
            return 1;
        }

        std::cout << "TCP session established on " << ip_text << ':' << kSessionPort << " (Joint mode)\n";

        florid::detail::UdpClient client{ip_text, port};
        florid::Arm arm{client, &tcp};
        arm.setMaxFrequencyHz(hz);

        arm.control([&](const florid::RobotState&, florid::RobotControl&)
        {
            const double t = ToSeconds(Clock::now());

            florid::JointPositions cmd{};
            for (int i = 0; i < 6; ++i)
            {
                const double freq = 0.2 + i * 0.15;
                const double phase = i * std::numbers::pi / 3.0;
                cmd.q[i] = static_cast<float>(0.5 * std::sin(2.0 * std::numbers::pi * freq * t + phase));
            }
            return cmd;
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to start sender: " << e.what() << '\n';
        return 1;
    }
}
