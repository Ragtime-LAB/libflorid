#ifndef FLORID_TCPCLIENT_HPP
#define FLORID_TCPCLIENT_HPP

#include <florid/detail/Transport.hpp>
#include <florid/protocols/cmd.hpp>
#include <florid/protocols/status.hpp>
#include <florid/protocols/dispatcher.hpp>
#include <asio.hpp>

#include <chrono>
#include <cstdint>
#include <string>

namespace florid::detail
{
    class TcpClient : public Transport
    {
    public:
        TcpClient(const std::string& ip, uint16_t port,
                  std::chrono::milliseconds timeout = std::chrono::seconds{5});
        ~TcpClient() override;

        bool send(const uint8_t* data, size_t size) override;
        void set_receive_callback(ReceiveFunctor callback, void* context) override;
        void poll() override {}

        bool configure_session(protocol::SessionMode mode,
                                uint16_t client_udp_port = 0);

        bool is_connected() const { return m_connected; }

    private:
        asio::io_context m_io_context;
        asio::ip::tcp::socket m_socket;

        std::chrono::milliseconds m_timeout;
        bool m_connected{false};

        ReceiveFunctor m_on_receive{nullptr};
        void* m_on_receive_context{nullptr};
    };
} // namespace florid::detail

#endif // FLORID_TCPCLIENT_HPP
