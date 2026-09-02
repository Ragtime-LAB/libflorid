#include "florid/detail/AstrialBulkTransport.hpp"

#include <astrial/Usb.hpp>

#include <utility>

namespace florid {

AstrialBulkTransport::AstrialBulkTransport(std::uint16_t s_vendor_id,
                                           std::uint16_t s_product_id,
                                           std::string s_serial_number) {
    m_config.usb.device.vendor_id = s_vendor_id;
    m_config.usb.device.product_id = s_product_id;
    m_config.usb.device.serial_number = std::move(s_serial_number);
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
        auto s_opened =
            wirelink::astrial::UsbBulkAdapter::open(s_link, m_config);
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

int AstrialBulkTransport::serviceWirelink() noexcept {
    return m_adapter ? m_adapter->service() : WL_ERR_INVALID_STATE;
}

void AstrialBulkTransport::quiesceWirelink() noexcept {
    m_adapter.reset();
    m_wake = nullptr;
    m_wake_context = nullptr;
}

AstrialBulkTransportStats AstrialBulkTransport::stats() const noexcept {
    if (!m_adapter) return {};

    wirelink::astrial::UsbBulkAdapterStats s_adapter{};
    m_adapter->get_stats(s_adapter);
    const UsbBulkStats s_usb = m_adapter->device().stats();
    return {
        .m_rx_claims = s_adapter.rx_claims,
        .m_rx_bytes = s_adapter.rx_bytes,
        .m_rx_pauses = s_adapter.rx_pauses,
        .m_tx_submissions = s_adapter.tx_submissions,
        .m_tx_completions = s_adapter.tx_completions,
        .m_tx_bytes = s_usb.bytes_transmitted,
        .m_activity_notifications = s_adapter.activity_notifications,
        .m_adapter_errors = s_adapter.errors,
        .m_usb_errors = s_usb.errors,
    };
}

void AstrialBulkTransport::s_onActivity(void* s_context) noexcept {
    if (s_context == nullptr) return;
    auto& s_self = *static_cast<AstrialBulkTransport*>(s_context);
    if (s_self.m_wake != nullptr) {
        s_self.m_wake(s_self.m_wake_context);
    }
}

std::vector<UsbBulkDeviceInfo> AstrialBulkTransport::listDevices() {
    std::vector<UsbBulkDeviceInfo> s_result;
    const auto s_devices = UsbBulkDevice::list_devices();
    if (!s_devices) return s_result;
    s_result.reserve(s_devices->size());
    for (const auto& s_device : *s_devices) {
        s_result.push_back(UsbBulkDeviceInfo{
            .m_vendor_id = s_device.vendor_id,
            .m_product_id = s_device.product_id,
            .m_bus_number = s_device.bus_number,
            .m_device_address = s_device.device_address,
            .m_port_path = s_device.port_path,
            .m_manufacturer = s_device.manufacturer,
            .m_product = s_device.product,
            .m_serial_number = s_device.serial_number,
        });
    }
    return s_result;
}

} // namespace florid
