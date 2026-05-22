#ifndef FLORID_UDPCLIENT_HPP
#define FLORID_UDPCLIENT_HPP

#include <florid/detail/Transport.hpp>
#include <asio.hpp>
#include <stddef.h>
#include <string>
#include <thread>

namespace florid::detail {

/**
 * @brief Host 平台专用：基于 ASIO 的 UDP 传输实现。
 *
 * 内部自行管理 io_context 和后台线程，对外仅暴露统一的 Transport 接口。
 */
class UdpClient : public Transport {
public:
    UdpClient(const std::string& ip, uint16_t port);
    ~UdpClient() override;

    bool send(const uint8_t* data, size_t size) override;
    void set_receive_callback(ReceiveFunctor callback, void* context) override;

    /**
     * @brief 对于 ASIO 实现，接收已由内部 io_context 线程驱动，无需外部 poll。
     */
    void poll() override {}

    uint16_t localPort() { return m_socket.local_endpoint().port(); }

private:
    void do_receive();

    asio::io_context m_io_context;
    std::thread m_thread;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_remote_endpoint;
    asio::ip::udp::endpoint m_sender_endpoint;
    alignas(8) uint8_t m_recv_buffer[1024]{};
    ReceiveFunctor m_on_receive{nullptr};
    void* m_on_receive_context{nullptr};
};

} // namespace florid::detail

#endif // FLORID_UDPCLIENT_HPP
