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

    // Fill a 4x4 column-major homogeneous transform with identity rotation + translation
    void MakePose(florid::core::CartesianPose& pose, float x, float y, float z)
    {
        // Column-major layout:
        //  0  4  8  12
        //  1  5  9  13
        //  2  6 10  14
        //  3  7 11  15
        pose.T[0]  = 1.0f; pose.T[4]  = 0.0f; pose.T[8]  = 0.0f; pose.T[12] = x;
        pose.T[1]  = 0.0f; pose.T[5]  = 1.0f; pose.T[9]  = 0.0f; pose.T[13] = y;
        pose.T[2]  = 0.0f; pose.T[6]  = 0.0f; pose.T[10] = 1.0f; pose.T[14] = z;
        pose.T[3]  = 0.0f; pose.T[7]  = 0.0f; pose.T[11] = 0.0f; pose.T[15] = 1.0f;
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

    std::cout << "Sending CartesianPose commands to " << address.to_string() << ':' << port << " at " << hz << " Hz\n";

    try
    {
        constexpr uint16_t kSessionPort = 6041;

        florid::detail::TcpClient tcp{ip_text, kSessionPort};
        if (!tcp.is_connected())
        {
            std::cerr << "Failed to connect TCP session to " << ip_text << ':' << kSessionPort << '\n';
            return 1;
        }

        if (!tcp.configure_session(florid::protocol::SessionMode::Point))
        {
            std::cerr << "TCP session handshake failed\n";
            return 1;
        }

        std::cout << "TCP session established on " << ip_text << ':' << kSessionPort << " (Point mode)\n";

        florid::detail::UdpClient client{ip_text, port};
        florid::Arm arm{client};
        arm.set_control_frequency_hz(hz);

        // Straight-line segment in Y direction, keeping X/Z fixed
        constexpr float kX = 300.0f;   // mm
        constexpr float kZ = 200.0f;   // mm
        constexpr float kYStart = -100.0f; // mm
        constexpr float kYEnd   =  100.0f; // mm
        constexpr double kPeriodSec = 4.0;

        arm.control([&](const florid::core::ArmStatus&)
        {
            const double t = ToSeconds(Clock::now());

            // Normalised progress 0..1..0 over the period
            const double phase = std::fmod(t, kPeriodSec);
            double s;
            if (phase < kPeriodSec * 0.5)
            {
                s = phase / (kPeriodSec * 0.5); // 0 -> 1
            }
            else
            {
                s = (kPeriodSec - phase) / (kPeriodSec * 0.5); // 1 -> 0
            }

            const float y = static_cast<float>(kYStart + (kYEnd - kYStart) * s);

            florid::core::CartesianPose pose{};
            MakePose(pose, kX, y, kZ);
            return pose;
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to start sender: " << e.what() << '\n';
        return 1;
    }
}
