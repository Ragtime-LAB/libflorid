#include <florid/Arm.hpp>
#include <florid/detail/UdpClient.hpp>
#include <florid/detail/TcpClient.hpp>
#include <asio.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

namespace
{
    void PrintUsage(const char* p) { std::cerr << "Usage: " << p << " <ip>\n"; }
    const char* mN(florid::RobotMode m) {
        switch(m){case florid::RobotMode::Init:return"INIT";case florid::RobotMode::Idle:return"IDLE";
        case florid::RobotMode::Running:return"RUN";case florid::RobotMode::Fault:return"FAULT";
        case florid::RobotMode::EStop:return"ESTOP";default:return"?";}
    }
}

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

        arm.read([&](const florid::RobotState& s) -> bool {
            std::cout<<"["<<mN(s.mode)<<"] q=[";
            for(int i=0;i<6;++i)std::cout<<s.q[i]<<(i<5?"|":"");
            std::cout<<"] errs="<<(s.errors?"0x":"")<<std::hex<<s.errors<<std::dec
                     <<" F_ext=["<<s.F_ext[0]<<","<<s.F_ext[1]<<","<<s.F_ext[2]<<"]"
                     <<"                    \r"<<std::flush;
            return s.mode!=florid::RobotMode::Fault && s.mode!=florid::RobotMode::EStop;
        });
    }
    catch(const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
