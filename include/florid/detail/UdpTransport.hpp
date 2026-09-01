#ifndef FLORID_DETAIL_UDP_TRANSPORT_HPP
#define FLORID_DETAIL_UDP_TRANSPORT_HPP

#include "florid/detail/Transport.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace florid {

// UDP transport for the arm control protocol.
//
// Binds a fixed local endpoint (ip:port) and learns the device's source
// endpoint from the first received datagram. All outgoing commands are sent
// back to that learned endpoint. UDP and USB both carry Wirelink v1
// COBS_STREAM + NONE bytes; datagram boundaries have no protocol meaning.
class UdpTransport : public Transport {
public:
    explicit UdpTransport(const std::string& s_bind_ip, std::uint16_t s_bind_port,
                          std::chrono::milliseconds s_first_datagram_timeout =
                              std::chrono::seconds(1));
    ~UdpTransport() override;

    UdpTransport(const UdpTransport&) = delete;
    UdpTransport& operator=(const UdpTransport&) = delete;
    UdpTransport(UdpTransport&&) noexcept;
    UdpTransport& operator=(UdpTransport&&) noexcept;

    bool send(const std::uint8_t* s_data, std::size_t s_size) override;

    void setReceiveCallback(ReceiveFunctor s_callback, void* s_context) override;

    void poll() override;

    bool isConnected() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace florid

#endif // FLORID_DETAIL_UDP_TRANSPORT_HPP
