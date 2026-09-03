#ifndef FLORID_DETAIL_ASTRIAL_BULK_TRANSPORT_HPP
#define FLORID_DETAIL_ASTRIAL_BULK_TRANSPORT_HPP

#include "florid/UsbDiscovery.hpp"
#include "florid/detail/Transport.hpp"

#include <wirelink/astrial/usb_bulk_adapter.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace florid {

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
    static constexpr std::uint16_t s_kDefaultVendorId = kDefaultUsbVendorId;
    static constexpr std::uint16_t s_kDefaultProductId = kDefaultUsbProductId;

    explicit AstrialBulkTransport(
        std::uint16_t s_vendor_id = s_kDefaultVendorId,
        std::uint16_t s_product_id = s_kDefaultProductId,
        std::string s_serial_number = {},
        std::vector<std::uint8_t> s_port_path = {});
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
    std::uint32_t wirelinkDeadlineHint(
        wl_time_ms_t s_now_ms) const noexcept override;

    [[nodiscard]] AstrialBulkTransportStats stats() const noexcept;
    [[nodiscard]] std::error_code lastError() const noexcept override {
        return m_last_error;
    }
    [[nodiscard]] TransportConnectionState connectionState()
        const noexcept override;

private:
    static void s_onActivity(void* s_context) noexcept;

    wirelink::astrial::UsbBulkAdapterConfig m_config{};
    WakeFunctor m_wake{};
    void* m_wake_context{};
    std::unique_ptr<wirelink::astrial::UsbBulkAdapter> m_adapter;
    std::error_code m_last_error;
};

} // namespace florid

#endif // FLORID_DETAIL_ASTRIAL_BULK_TRANSPORT_HPP
