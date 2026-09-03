#include "florid/detail/AstrialBulkTransport.hpp"

#include <astrial/Usb.hpp>

#include <utility>

namespace florid {

AstrialBulkTransport::AstrialBulkTransport(std::uint16_t s_vendor_id,
                                           std::uint16_t s_product_id,
                                           std::string s_serial_number,
                                           std::vector<std::uint8_t> s_port_path) {
    m_config.usb.device.vendor_id = s_vendor_id;
    m_config.usb.device.product_id = s_product_id;
    m_config.usb.device.serial_number = std::move(s_serial_number);
    m_config.usb.device.port_path = std::move(s_port_path);
    m_config.usb.bulk_interface.interface_number = 0;
    m_config.usb.bulk_interface.endpoint_in = 0x81;
    m_config.usb.bulk_interface.endpoint_out = 0x01;
    m_config.usb.read_queue_depth = 1;
    m_config.maximum_read_size = 512;
    m_config.wake_policy =
        wirelink::astrial::UsbBulkWakePolicy::AllCompletions;
    m_config.activity_callback = s_onActivity;
    m_config.activity_user_data = this;
}

AstrialBulkTransport::~AstrialBulkTransport() {
    quiesceWirelink();
}

int AstrialBulkTransport::attachWirelink(wl_ctx_t& s_link,
                                         WakeFunctor s_wake,
                                         void* s_wake_context) noexcept {
    if (m_adapter || s_wake == nullptr) return WL_ERR_INVALID_STATE;
    m_wake = s_wake;
    m_wake_context = s_wake_context;
    try {
        m_last_error.clear();
        auto s_opened =
            wirelink::astrial::UsbBulkAdapter::open(s_link, m_config);
        if (!s_opened) {
            m_last_error = s_opened.error();
            m_wake = nullptr;
            m_wake_context = nullptr;
            return WL_ERR_IO;
        }
        m_adapter = std::move(s_opened.value());
        return WL_OK;
    } catch (...) {
        m_last_error = make_error_code(UsbError::IoError);
        m_wake = nullptr;
        m_wake_context = nullptr;
        return WL_ERR_IO;
    }
}

int AstrialBulkTransport::serviceWirelink() noexcept {
    return m_adapter ? m_adapter->service() : WL_ERR_INVALID_STATE;
}

void AstrialBulkTransport::quiesceWirelink() noexcept {
    if (m_adapter) m_adapter->quiesce();
    m_adapter.reset();
    m_wake = nullptr;
    m_wake_context = nullptr;
}

std::uint32_t AstrialBulkTransport::wirelinkDeadlineHint(
    wl_time_ms_t s_now_ms) const noexcept {
    return m_adapter ? m_adapter->deadline_hint(s_now_ms)
                     : WL_POLL_NO_DEADLINE_MS;
}

AstrialBulkTransportStats AstrialBulkTransport::stats() const noexcept {
    if (!m_adapter) return {};

    wl_adapter_stats_t s_adapter{};
    m_adapter->get_common_stats(s_adapter);
    const UsbBulkStats s_usb = m_adapter->device().stats();
    return {
        .m_rx_claims = s_adapter.rx_units,
        .m_rx_bytes = s_adapter.rx_bytes,
        .m_rx_pauses = s_adapter.rx_backpressure,
        .m_tx_submissions = s_adapter.tx_units,
        .m_tx_completions = s_adapter.tx_completions,
        .m_tx_bytes = s_adapter.tx_bytes,
        .m_activity_notifications = s_adapter.activity_notifications,
        .m_adapter_errors = s_adapter.errors,
        .m_usb_errors = s_usb.errors,
    };
}

TransportConnectionState AstrialBulkTransport::connectionState()
    const noexcept {
    if (!m_adapter) return TransportConnectionState::kClosed;
    switch (m_adapter->device().state()) {
        case UsbState::Connected:
            return TransportConnectionState::kConnected;
        case UsbState::Disconnected:
            return TransportConnectionState::kDisconnected;
        case UsbState::Reconnecting:
            return TransportConnectionState::kReconnecting;
        case UsbState::Closed:
            return TransportConnectionState::kClosed;
    }
    return TransportConnectionState::kUnknown;
}

void AstrialBulkTransport::s_onActivity(void* s_context) noexcept {
    if (s_context == nullptr) return;
    auto& s_self = *static_cast<AstrialBulkTransport*>(s_context);
    if (s_self.m_wake != nullptr) {
        s_self.m_wake(s_self.m_wake_context);
    }
}

} // namespace florid
