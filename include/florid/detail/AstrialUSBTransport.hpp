#ifndef FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP
#define FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP

#include "florid/detail/ReceiveCallbackGate.hpp"
#include "florid/detail/Transport.hpp"

#include <memory>
#include <string>
#include <vector>

class Serial;

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

  bool send(const std::uint8_t *s_data, std::size_t s_size) override;

  void setReceiveCallback(ReceiveFunctor s_callback, void *s_context) override;

  static std::vector<UsbDeviceInfo> listDevices();

private:
  void s_installReceiveHandler();

  std::unique_ptr<Serial> m_serial;
  detail::ReceiveCallbackGate m_receive_callback;
};

} // namespace florid

#endif // FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP
