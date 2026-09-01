#include "florid/detail/AstrialUSBTransport.hpp"

#include "astrial/Serial.hpp"
#include "astrial/SerialBuilder.hpp"
#include "astrial/Types.hpp"


namespace florid {

AstrialUSBTransport::AstrialUSBTransport(const std::string &s_port_path,
  std::uint32_t s_baud_rate) {
  auto s_result = Serial::builder()//打开串口
                      .baud_rate(s_baud_rate)//设置波特率
                      .parity(Parity::None)//设置校验位
                      .data_bits(DataBits::Eight)//设置数据位
                      .stop_bits(StopBits::One)//设置停止位
                      .open(s_port_path);//打开串口

  if (!s_result) {
    throw std::runtime_error("Failed to open USB device: " + s_port_path +
                             " (" + s_result.error().message() + ")");
  }

  m_serial = std::make_unique<Serial>(std::move(s_result.value()));

}

AstrialUSBTransport::~AstrialUSBTransport() {
  m_receive_callback.clear();
  if (m_serial) {
    m_serial->close();
  }
}

bool AstrialUSBTransport::send(const std::uint8_t *s_data, std::size_t s_size) {
  if (!m_serial)
    return false;
  auto s_r = m_serial->write(std::span<const std::uint8_t>(s_data, s_size));
  return s_r.has_value();
}

void AstrialUSBTransport::setReceiveCallback(ReceiveFunctor s_callback,
                                             void *s_context) {
  m_receive_callback.set(s_callback, s_context);

  if (s_callback && m_serial) {
    s_installReceiveHandler();
  }
}

void AstrialUSBTransport::s_installReceiveHandler() {
  m_serial->on_data([this](std::span<const std::uint8_t> s_data) {
    m_receive_callback.invoke(s_data.data(), s_data.size());
  });
}

std::vector<UsbDeviceInfo> AstrialUSBTransport::listDevices() {
  std::vector<UsbDeviceInfo> s_devices;
  auto s_ports = Serial::list_ports();
  for (const auto &s_port : s_ports) {
    UsbDeviceInfo s_info;
    s_info.m_port_name = s_port.port_name;
    s_info.m_description = s_port.description;
    s_info.m_vendor_id = s_port.vendor_id;
    s_info.m_product_id = s_port.product_id;
    s_info.m_serial_number = s_port.serial_number;
    s_info.m_manufacturer = s_port.manufacturer;
    s_devices.push_back(std::move(s_info));
  }
  return s_devices;
}

} // namespace florid
