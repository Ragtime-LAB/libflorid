#ifndef FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP
#define FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP

#include "florid/detail/Transport.hpp"

#include <wirelink/astrial/serial_adapter.hpp>

#include <memory>
#include <string>
#include <vector>

namespace florid {

struct UsbDeviceInfo {
  std::string m_port_name;
  std::string m_description;
  std::uint16_t m_vendor_id;
  std::uint16_t m_product_id;
  std::string m_serial_number;
  std::string m_manufacturer;
};

class AstrialUSBTransport : public Transport {
public:
  explicit AstrialUSBTransport(const std::string &s_port_path,
                               std::uint32_t s_baud_rate = 115200);
  ~AstrialUSBTransport() override;

  AstrialUSBTransport(const AstrialUSBTransport &) = delete;
  AstrialUSBTransport &operator=(const AstrialUSBTransport &) = delete;
  AstrialUSBTransport(AstrialUSBTransport &&) = delete;
  AstrialUSBTransport &operator=(AstrialUSBTransport &&) = delete;

  bool send(const std::uint8_t*, std::size_t) override { return false; }
  void setReceiveCallback(ReceiveFunctor, void*) override {}
  bool usesDirectWirelink() const noexcept override { return true; }
  int attachWirelink(wl_ctx_t& s_link, WakeFunctor s_wake,
                     void* s_wake_context) noexcept override;
  int serviceWirelink() noexcept override;
  void quiesceWirelink() noexcept override;
  std::uint32_t wirelinkDeadlineHint(
      wl_time_ms_t s_now_ms) const noexcept override;

  static std::vector<UsbDeviceInfo> listDevices();

private:
  static void s_onActivity(void* s_context) noexcept;

  wirelink::astrial::SerialConfig m_config;
  WakeFunctor m_wake{};
  void* m_wake_context{};
  std::unique_ptr<wirelink::astrial::SerialAdapter> m_adapter;
};

} // namespace florid

#endif // FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP
