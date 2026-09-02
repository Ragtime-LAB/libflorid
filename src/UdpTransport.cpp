#include "florid/detail/UdpTransport.hpp"

namespace florid {

UdpTransport::UdpTransport(
    const std::string& s_bind_ip, std::uint16_t s_bind_port,
    std::chrono::milliseconds s_first_datagram_timeout) {
    (void)s_first_datagram_timeout;
    m_config.bind_address = s_bind_ip;
    m_config.bind_port = s_bind_port;
    m_config.maximum_datagram_size = 2048;
    m_config.poll_interval = std::chrono::milliseconds{1};
    m_config.learn_peer_from_first_datagram = true;
}

UdpTransport::~UdpTransport() {
    quiesceWirelink();
}

int UdpTransport::attachWirelink(wl_ctx_t& s_link, WakeFunctor s_wake,
                                 void* s_wake_context) noexcept {
    if (m_adapter || s_wake == nullptr) return WL_ERR_INVALID_STATE;
    (void)s_wake_context;
    std::error_code s_error;
    try {
        m_adapter = wirelink::asio::UdpAdapter::open(s_link, m_config, s_error);
    } catch (...) {
        return WL_ERR_IO;
    }
    if (!m_adapter || s_error) return WL_ERR_IO;
    return WL_OK;
}

int UdpTransport::serviceWirelink() noexcept {
    return m_adapter ? m_adapter->service() : WL_ERR_INVALID_STATE;
}

void UdpTransport::quiesceWirelink() noexcept {
    if (m_adapter) m_adapter->quiesce();
    m_adapter.reset();
}

std::uint32_t UdpTransport::wirelinkDeadlineHint(
    wl_time_ms_t s_now_ms) const noexcept {
    return m_adapter ? m_adapter->deadline_hint(s_now_ms)
                     : WL_POLL_NO_DEADLINE_MS;
}

} // namespace florid
