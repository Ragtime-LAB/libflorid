#include "florid/detail/UdpTransport.hpp"

#include "florid/detail/ReceiveCallbackGate.hpp"

#include <asio.hpp>

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace florid {

namespace {
constexpr std::size_t s_kMaxDatagramSize = 2048;

#ifdef _WIN32
void s_disableUdpConnReset(asio::ip::udp::socket& s_socket) {
    // Windows surfaces ICMP port-unreachable on unconnected UDP sockets as
    // WSAECONNRESET on the next operation. Disable that so a stray datagram
    // to a not-yet-bound peer cannot abort our receive loop.
    constexpr DWORD s_kSioUdpConnReset = 0x9800000C;
    DWORD s_zero = 0;
    DWORD s_returned = 0;
    (void)::WSAIoctl(s_socket.native_handle(), s_kSioUdpConnReset, &s_zero,
                     sizeof(s_zero), nullptr, 0, &s_returned, nullptr, nullptr);
}
#endif
} // namespace

struct UdpTransport::Impl {
    asio::io_context m_ctx;
    asio::ip::udp::socket m_socket;
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_work_guard;
    std::jthread m_thread;

    std::array<std::uint8_t, s_kMaxDatagramSize> m_rx_buffer{};
    asio::ip::udp::endpoint m_remote_endpoint;

    // Learned device endpoint (from first received datagram).
    std::mutex m_peer_mutex;
    asio::ip::udp::endpoint m_peer;
    std::atomic<bool> m_peer_known{false};
    std::condition_variable m_peer_cv;

    // Receive callback (installed by ArmImpl).
    detail::ReceiveCallbackGate m_receive_callback;

    std::atomic<bool> m_closed{false};

    Impl(const std::string& s_bind_ip, std::uint16_t s_bind_port)
        : m_ctx(1), m_socket(m_ctx) {
        asio::ip::udp::endpoint s_local(asio::ip::make_address(s_bind_ip), s_bind_port);
        m_socket.open(s_local.protocol());
#ifdef _WIN32
        s_disableUdpConnReset(m_socket);
#endif
        m_socket.bind(s_local);
    }

    void close() {
        m_receive_callback.clear();
        if (m_closed.exchange(true, std::memory_order_acq_rel)) return;
        asio::error_code s_ec;
        m_socket.close(s_ec);
        m_ctx.stop();
        if (m_thread.joinable()) m_thread.join();
    }

    void start_receive() {
        if (m_closed.load(std::memory_order_acquire)) return;

        try {
            m_socket.async_receive_from(
                asio::buffer(m_rx_buffer), m_remote_endpoint,
                [this](const asio::error_code& s_ec, std::size_t s_bytes) {
                    if (s_ec) {
                        if (s_ec != asio::error::operation_aborted) {
                            start_receive();
                        }
                        return;
                    }

                    bool s_peer_published = false;
                    if (!m_peer_known.load(std::memory_order_acquire)) {
                        std::lock_guard<std::mutex> s_lock(m_peer_mutex);
                        if (!m_peer_known.load(std::memory_order_relaxed)) {
                            m_peer = m_remote_endpoint;
                            m_peer_known.store(true, std::memory_order_release);
                            s_peer_published = true;
                        }
                    }
                    if (s_peer_published) m_peer_cv.notify_one();

                    if (s_bytes > 0) {
                        m_receive_callback.invoke(m_rx_buffer.data(), s_bytes);
                    }

                    if (!m_closed.load(std::memory_order_acquire)) {
                        start_receive();
                    }
                });
        } catch (...) {
            // Socket was closed concurrently during shutdown.
        }
    }
};

UdpTransport::UdpTransport(const std::string& s_bind_ip, std::uint16_t s_bind_port,
                           std::chrono::milliseconds s_first_datagram_timeout)
    : m_impl(std::make_unique<Impl>(s_bind_ip, s_bind_port)) {
    auto& s_impl = *m_impl;

    s_impl.m_work_guard =
        std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
            s_impl.m_ctx.get_executor());
    s_impl.m_thread = std::jthread([&s_impl] {
#ifdef _WIN32
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
        try {
            s_impl.m_ctx.run();
        } catch (const std::exception& s_e) {
            std::fprintf(stderr, "UdpTransport io thread exception: %s\n", s_e.what());
        } catch (...) {
            std::fprintf(stderr, "UdpTransport io thread unknown exception\n");
        }
    });

    s_impl.start_receive();

    // Block until the device's first datagram reveals its source endpoint so
    // that ArmImpl's handshake sends go to the correct peer.
    std::unique_lock<std::mutex> s_lock(s_impl.m_peer_mutex);
    if (!s_impl.m_peer_cv.wait_for(s_lock, s_first_datagram_timeout,
                                   [&s_impl] {
                                       return s_impl.m_peer_known.load(
                                           std::memory_order_acquire);
                                   })) {
        s_lock.unlock();
        s_impl.close();
        throw std::runtime_error("UdpTransport: no datagram received from device within "
                                 + std::to_string(s_first_datagram_timeout.count()) + " ms at "
                                 + s_bind_ip + ":" + std::to_string(s_bind_port));
    }
}

UdpTransport::~UdpTransport() {
    if (m_impl) {
        m_impl->close();
    }
}

bool UdpTransport::send(const std::uint8_t* s_data, std::size_t s_size) {
    if (!m_impl || s_size == 0) return false;

    asio::error_code s_ec;
    std::size_t s_sent = m_impl->m_socket.send_to(
        asio::buffer(s_data, s_size), m_impl->m_peer, 0, s_ec);
    return !s_ec && s_sent == s_size;
}

void UdpTransport::setReceiveCallback(ReceiveFunctor s_callback, void* s_context) {
    if (!m_impl) return;
    m_impl->m_receive_callback.set(s_callback, s_context);
}

} // namespace florid
