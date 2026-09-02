#ifndef FLORID_DETAIL_ASTRIAL_BULK_TRANSPORT_HPP
#define FLORID_DETAIL_ASTRIAL_BULK_TRANSPORT_HPP

#include "florid/detail/Transport.hpp"

#include <wirelink/astrial/usb_bulk_adapter.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace florid {

struct UsbBulkDeviceInfo {
    std::uint16_t m_vendor_id{};
    std::uint16_t m_product_id{};
    std::uint8_t m_bus_number{};
    std::uint8_t m_device_address{};
    std::vector<std::uint8_t> m_port_path;
    std::string m_manufacturer;
    std::string m_product;
    std::string m_serial_number;
};

struct AstrialBulkTransportStats {
    std::uint64_t m_rx_claims{};
    std::uint64_t m_rx_bytes{};
    std::uint64_t m_rx_pauses{};
    std::uint64_t m_tx_submissions{};
    std::uint64_t m_tx_completions{};
    std::uint64_t m_tx_bytes{};
    std::uint64_t m_activity_notifications{};
    std::uint64_t m_adapter_errors{};
    std::uint64_t m_usb_errors{};
};

class AstrialBulkTransport final : public Transport {
public:
    static constexpr std::uint16_t s_kDefaultVendorId = 0x2fe3;
    static constexpr std::uint16_t s_kDefaultProductId = 0x574c;

    explicit AstrialBulkTransport(
        std::uint16_t s_vendor_id = s_kDefaultVendorId,
        std::uint16_t s_product_id = s_kDefaultProductId,
        std::string s_serial_number = {});
    ~AstrialBulkTransport() override;

    AstrialBulkTransport(const AstrialBulkTransport&) = delete;
    AstrialBulkTransport& operator=(const AstrialBulkTransport&) = delete;

    bool send(const std::uint8_t*, std::size_t) override { return false; }
    void setReceiveCallback(ReceiveFunctor, void*) override {}

    bool usesDirectWirelink() const noexcept override { return true; }
    int attachWirelink(wl_ctx_t& s_link, WakeFunctor s_wake,
                       void* s_wake_context) noexcept override;
    int serviceWirelink() noexcept override;
    void quiesceWirelink() noexcept override;

    [[nodiscard]] AstrialBulkTransportStats stats() const noexcept;

    static std::vector<UsbBulkDeviceInfo> listDevices();

private:
    static void s_onActivity(void* s_context) noexcept;

    wirelink::astrial::UsbBulkAdapterConfig m_config{};
    WakeFunctor m_wake{};
    void* m_wake_context{};
    std::unique_ptr<wirelink::astrial::UsbBulkAdapter> m_adapter;
};

} // namespace florid

#endif // FLORID_DETAIL_ASTRIAL_BULK_TRANSPORT_HPP
