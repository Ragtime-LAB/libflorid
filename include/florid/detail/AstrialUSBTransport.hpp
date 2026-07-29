#ifndef FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP
#define FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP

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

  TxSubmitResult submit(TxClass s_class, std::span<const std::uint8_t> s_data,
                        TxCompletion s_completion = {}) override;

  void setReceiveCallback(ReceiveFunctor s_callback, void *s_context) override;

  void poll() override;

  bool isConnected() const;

  static std::vector<UsbDeviceInfo> listDevices();

private:
  struct TxState;

  std::unique_ptr<Serial> m_serial;
  std::unique_ptr<TxState> m_tx_state;
  ReceiveFunctor m_recv_callback{nullptr};
  void *m_recv_context{nullptr};
};

} // namespace florid

#endif // FLORID_DETAIL_ASTRIAL_USB_TRANSPORT_HPP
