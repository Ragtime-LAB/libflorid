#include "florid/detail/AstrialUSBTransport.hpp"

#include <astrial/Serial.hpp>

namespace florid {

AstrialUSBTransport::AstrialUSBTransport(const std::string& s_port_path,
                                         std::uint32_t s_baud_rate) {
    m_config.port = s_port_path;
    m_config.baud_rate = s_baud_rate;
    m_config.auto_reconnect = true;
    m_config.activity_callback = s_onActivity;
    m_config.activity_user_data = this;
}

AstrialUSBTransport::~AstrialUSBTransport() {
    quiesceWirelink();
}

int AstrialUSBTransport::attachWirelink(wl_ctx_t& s_link,
                                        WakeFunctor s_wake,
                                        void* s_wake_context) noexcept {
    if (m_adapter || s_wake == nullptr) return WL_ERR_INVALID_STATE;
    m_wake = s_wake;
    m_wake_context = s_wake_context;
    try {
        auto s_opened =
            wirelink::astrial::SerialAdapter::open(s_link, m_config);
        if (!s_opened) {
            m_wake = nullptr;
            m_wake_context = nullptr;
            return WL_ERR_IO;
        }
        m_adapter = std::move(s_opened.value());
        return WL_OK;
    } catch (...) {
        m_wake = nullptr;
        m_wake_context = nullptr;
        return WL_ERR_IO;
    }
}

int AstrialUSBTransport::serviceWirelink() noexcept {
    return m_adapter ? m_adapter->service() : WL_ERR_INVALID_STATE;
}

void AstrialUSBTransport::quiesceWirelink() noexcept {
    if (m_adapter) m_adapter->quiesce();
    m_adapter.reset();
    m_wake = nullptr;
    m_wake_context = nullptr;
}

std::uint32_t AstrialUSBTransport::wirelinkDeadlineHint(
    wl_time_ms_t s_now_ms) const noexcept {
    return m_adapter ? m_adapter->deadline_hint(s_now_ms)
                     : WL_POLL_NO_DEADLINE_MS;
}

void AstrialUSBTransport::s_onActivity(void* s_context) noexcept {
    if (s_context == nullptr) return;
    auto& s_self = *static_cast<AstrialUSBTransport*>(s_context);
    if (s_self.m_wake != nullptr) s_self.m_wake(s_self.m_wake_context);
}

std::vector<UsbDeviceInfo> AstrialUSBTransport::listDevices() {
    std::vector<UsbDeviceInfo> s_devices;
    auto s_ports = Serial::list_ports();
    for (const auto& s_port : s_ports) {
        s_devices.push_back(UsbDeviceInfo{
            .m_port_name = s_port.port_name,
            .m_description = s_port.description,
            .m_vendor_id = s_port.vendor_id,
            .m_product_id = s_port.product_id,
            .m_serial_number = s_port.serial_number,
            .m_manufacturer = s_port.manufacturer,
        });
    }
    return s_devices;
}

} // namespace florid
