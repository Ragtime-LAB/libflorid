#ifndef FLORID_DETAIL_UDP_TRANSPORT_HPP
#define FLORID_DETAIL_UDP_TRANSPORT_HPP

#include "florid/detail/Transport.hpp"

#include <wirelink/asio/udp_adapter.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace florid {

// UDP transport for the arm control protocol.
//
// Binds a fixed local endpoint and delegates peer learning plus direct
// Wirelink RX/TX ownership to the reusable Asio adapter.
class UdpTransport : public Transport {
public:
    explicit UdpTransport(const std::string& s_bind_ip, std::uint16_t s_bind_port,
                          std::chrono::milliseconds s_first_datagram_timeout =
                              std::chrono::seconds(1));
    ~UdpTransport() override;

    UdpTransport(const UdpTransport&) = delete;
    UdpTransport& operator=(const UdpTransport&) = delete;
    UdpTransport(UdpTransport&&) = delete;
    UdpTransport& operator=(UdpTransport&&) = delete;

    bool send(const std::uint8_t*, std::size_t) override { return false; }
    void setReceiveCallback(ReceiveFunctor, void*) override {}
    bool usesDirectWirelink() const noexcept override { return true; }
    int attachWirelink(wl_ctx_t& s_link, WakeFunctor s_wake,
                       void* s_wake_context) noexcept override;
    int serviceWirelink() noexcept override;
    void quiesceWirelink() noexcept override;
    std::uint32_t wirelinkDeadlineHint(
        wl_time_ms_t s_now_ms) const noexcept override;

private:
    wirelink::asio::UdpAdapterConfig m_config;
    std::unique_ptr<wirelink::asio::UdpAdapter> m_adapter;
};

} // namespace florid

#endif // FLORID_DETAIL_UDP_TRANSPORT_HPP
