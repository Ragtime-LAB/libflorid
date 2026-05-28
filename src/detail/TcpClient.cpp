#include <florid/detail/TcpClient.hpp>

#include <florid/protocols/version.hpp>

#include <cstring>
#include <system_error>

namespace florid::detail
{
    namespace
    {
        using florid::protocol::CURRENT_PROTOCOL_VERSION;
        using florid::protocol::Dispatcher;
        using florid::protocol::PacketRegistry;
        using florid::protocol::SessionCfgPacket;
        using florid::protocol::SessionMode;
        using florid::protocol::SessionStatus;
        using florid::protocol::SessionStatusPacket;
    }

    TcpClient::TcpClient(const std::string& ip, const uint16_t port,
                         const std::chrono::milliseconds timeout)
        : m_socket(m_io_context)
        , m_timeout(timeout)
    {
        const asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip), port);

        std::error_code ec;
        m_socket.connect(endpoint, ec);
        if (ec)
        {
            return;
        }

        m_connected = true;
    }

    TcpClient::~TcpClient()
    {
        if (m_connected)
        {
            std::error_code ec;
            m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            m_socket.close(ec);
            m_connected = false;
        }
    }

    bool TcpClient::send(const uint8_t* data, const size_t size)
    {
        if (!m_connected || data == nullptr || size == 0)
        {
            return false;
        }

        std::error_code ec;
        asio::write(m_socket, asio::buffer(data, size), ec);
        return !ec;
    }

    void TcpClient::set_receive_callback(const ReceiveFunctor callback, void* context)
    {
        m_on_receive = callback;
        m_on_receive_context = context;
    }

    bool TcpClient::configure_session(const SessionMode mode,
                                       const uint16_t client_udp_port)
    {
        if (!m_connected)
        {
            return false;
        }

        SessionCfgPacket cfg{};
        cfg.header.length = static_cast<uint16_t>(sizeof(SessionCfgPacket));
        cfg.header.seq_num = 1;
        cfg.timestamp_us = 0;
        cfg.version = CURRENT_PROTOCOL_VERSION.raw;
        cfg.mode = mode;
        cfg.client_udp_port = client_udp_port;

        static_assert(sizeof(cfg) == 24, "SessionCfgPacket size mismatch");

        std::error_code ec;
        asio::write(m_socket, asio::buffer(&cfg, sizeof(cfg)), ec);
        if (ec)
        {
            return false;
        }

        alignas(8) uint8_t recv_buf[sizeof(SessionStatusPacket)]{};
        static_assert(sizeof(recv_buf) == 16, "SessionStatusPacket buffer size mismatch");

        asio::read(m_socket, asio::buffer(recv_buf, sizeof(recv_buf)), ec);
        if (ec)
        {
            return false;
        }

        bool success = false;
        Dispatcher<PacketRegistry<SessionStatusPacket>>::dispatch(
            recv_buf, sizeof(recv_buf),
            [&success](const SessionStatusPacket& pkt)
            {
                if (pkt.status == SessionStatus::SUCCESS &&
                    florid::protocol::versions_compatible(pkt.version, CURRENT_PROTOCOL_VERSION.raw))
                {
                    success = true;
                }
            });

        return success;
    }
} // namespace florid::detail
