#include <florid/detail/UdpClient.hpp>

namespace florid::detail {

UdpClient::UdpClient(const std::string& ip, const uint16_t port)
    : m_socket(m_io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)),
      m_remote_endpoint(asio::ip::make_address(ip), port)
{
    const asio::socket_base::receive_buffer_size recv_size(1024 * 1024);
    m_socket.set_option(recv_size);

    do_receive(); // 启动异步接收链
    m_thread = std::thread([this]() { m_io_context.run(); });
}

UdpClient::~UdpClient()
{
    {
        std::error_code ec;
        m_socket.cancel(ec);
        m_socket.close(ec);
        m_io_context.stop();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool UdpClient::send(const uint8_t* data, const size_t size)
{
    if (data == nullptr || size == 0) return false;
    std::error_code ec;
    m_socket.send_to(asio::buffer(data, size), m_remote_endpoint, 0, ec);
    return !ec;
}

void UdpClient::set_receive_callback(const ReceiveFunctor callback, void* context)
{
    m_on_receive = callback;
    m_on_receive_context = context;
}

void UdpClient::do_receive()
{
    m_socket.async_receive_from(
        asio::buffer(m_recv_buffer), m_sender_endpoint,
        [this](const std::error_code ec, const size_t bytes_recvd)
        {
            if (!ec && bytes_recvd > 0 && m_on_receive) {
                m_on_receive(m_on_receive_context, m_recv_buffer, bytes_recvd);
            }
            if (ec != asio::error::operation_aborted) {
                do_receive();
            }
        });
}

} // namespace florid::detail
