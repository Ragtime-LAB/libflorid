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
#include <random>
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

    bool ParseJitterPercent(const char* text, double& percent)
    {
        try
        {
            size_t processed = 0;
            const std::string value{text};
            percent = std::stod(value, &processed);
            return processed == value.size() && percent >= 0.0 && percent < 100.0;
        }
        catch (...)
        {
            return false;
        }
    }

    void PrintUsage(const char* program)
    {
        std::cerr << "Usage: " << program << " <ip> <port> <hz> <jitter_percent>\n"
            << "  ip   : target IPv4/IPv6 address\n"
            << "  port : UDP port in range [1, 65535]\n"
            << "  hz   : base send frequency, must be > 0\n"
            << "  jitter_percent : frequency jitter in [0, 100)\n";
    }

    double ToSeconds(const Clock::time_point& now)
    {
        const auto elapsed = std::chrono::duration<double>(now.time_since_epoch());
        return elapsed.count();
    }

    constexpr double kHelixV = 20.0;

    double CurveHelixX(const double t)
    {
        return kHelixV * t;
    }

    constexpr double kJitterA = 100.0;
    constexpr double kJitterHz = 1.0;

    double CurveJitterSine(const double t)
    {
        return kJitterA * std::sin(2.0 * std::numbers::pi * kJitterHz * t);
    }

    constexpr double kStepLow = 0.0;
    constexpr double kStepHigh = 20.0;
    constexpr double kStepPeriodSec = 2.0;

    double CurveTriangle(const double t)
    {
        const double phase = std::fmod(t, kStepPeriodSec);
        constexpr double half = kStepPeriodSec * 0.5;
        const double ratio = (phase < half) ? (phase / half) : ((kStepPeriodSec - phase) / half);
        return kStepLow + (kStepHigh - kStepLow) * ratio;
    }
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    const std::string ip_text = argv[1];
    unsigned short port = 0;
    double hz = 0.0;
    double jitter_percent = 0.0;

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

    if (!ParseJitterPercent(argv[4], jitter_percent))
    {
        std::cerr << "Invalid jitter percent: " << argv[4] << '\n';
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

    std::cout << "Sending CartesianPose commands to " << address.to_string() << ':' << port << " at " << hz
        << " Hz with " << jitter_percent << "% jitter\n";

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

        std::cout << "TCP session established on " << ip_text << ':' << kSessionPort << '\n';

        florid::detail::UdpClient client{ip_text, port};
        florid::Arm arm{client, &tcp};
        arm.setMaxFrequencyHz(0.0);

        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<double> jitter_scale_distribution(
            1.0 - jitter_percent / 100.0,
            1.0 + jitter_percent / 100.0);

        auto next_send = Clock::now();

        arm.control([&](const florid::RobotState&, florid::RobotControl&)
        {
            const double jitter_scale = jitter_scale_distribution(rng);
            const double period_sec = (1.0 / hz) * jitter_scale;
            const auto period_duration = std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>(period_sec));

            next_send += period_duration;
            std::this_thread::sleep_until(next_send);

            const double t = ToSeconds(Clock::now());

            florid::CartesianPose pose{};
            pose.T[0] = static_cast<float>(CurveHelixX(t));
            pose.T[1] = static_cast<float>(CurveJitterSine(t));
            pose.T[2] = static_cast<float>(CurveTriangle(t));
            pose.T[3] = static_cast<float>(CurveHelixX(0.5 * t));
            pose.T[4] = static_cast<float>(CurveJitterSine(0.7 * t));
            pose.T[5] = static_cast<float>(CurveTriangle(t + 0.25));
            pose.T[6] = static_cast<float>(CurveHelixX(0.2 * t));
            pose.T[7] = static_cast<float>(CurveJitterSine(1.3 * t));
            pose.T[8] = static_cast<float>(CurveTriangle(t + 0.5));
            pose.T[9] = static_cast<float>(CurveHelixX(0.1 * t));
            pose.T[10] = static_cast<float>(CurveJitterSine(0.3 * t));
            pose.T[11] = static_cast<float>(CurveTriangle(t + 0.75));
            return pose;
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to start sender: " << e.what() << '\n';
        return 1;
    }
}
