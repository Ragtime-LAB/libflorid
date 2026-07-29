#include "florid/detail/AstrialUSBTransport.hpp"
#include "florid/detail/LatestMailbox.hpp"

#include "astrial/Serial.hpp"
#include "astrial/SerialBuilder.hpp"
#include "astrial/Types.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>

namespace florid {

namespace {

constexpr std::size_t kMaxTxFrameSize = 4096;
constexpr std::size_t kReliableQueueCapacity = 64;

TxCompletionStatus completionStatus(const std::error_code &s_error) {
  if (!s_error) return TxCompletionStatus::Completed;
  if (s_error == SerialError::DeviceDisconnected) {
    return TxCompletionStatus::Disconnected;
  }
  if (s_error == std::make_error_code(std::errc::operation_canceled)) {
    return TxCompletionStatus::Cancelled;
  }
  return TxCompletionStatus::IoError;
}

} // namespace

struct AstrialUSBTransport::TxState {
  struct ReliableFrame {
    std::array<std::uint8_t, kMaxTxFrameSize> m_data{};
    std::size_t m_size{0};
    TxCompletion m_completion;
  };

  detail::LatestMailbox<kMaxTxFrameSize> m_control_mailbox;
  std::mutex m_reliable_mutex;
  std::deque<ReliableFrame> m_reliable_queue;

  std::mutex m_wake_mutex;
  std::condition_variable m_wake_cv;
  std::atomic<std::uint64_t> m_wake_epoch{0};
  std::atomic<bool> m_write_in_flight{false};
  std::atomic_flag m_control_producer_busy = ATOMIC_FLAG_INIT;
  std::jthread m_dispatch_thread;

  void wake() noexcept {
    m_wake_epoch.fetch_add(1, std::memory_order_release);
    m_wake_cv.notify_one();
  }

  bool tryTakeReliable(ReliableFrame &s_frame) {
    std::lock_guard<std::mutex> s_lock(m_reliable_mutex);
    if (m_reliable_queue.empty()) return false;
    s_frame = std::move(m_reliable_queue.front());
    m_reliable_queue.pop_front();
    return true;
  }
};

AstrialUSBTransport::AstrialUSBTransport(const std::string &s_port_path,
                                         std::uint32_t s_baud_rate) {
  auto s_result = Serial::builder()
                      .baud_rate(s_baud_rate)
                      .parity(Parity::None)
                      .stop_bits(StopBits::One)
                      .open(s_port_path);

  if (!s_result) {
    throw std::runtime_error("Failed to open USB device: " + s_port_path +
                             " (" + s_result.error().message() + ")");
  }

  m_serial = std::make_unique<Serial>(std::move(s_result.value()));
  m_tx_state = std::make_unique<TxState>();
  auto *const s_serial = m_serial.get();
  auto *const s_state = m_tx_state.get();
  s_state->m_dispatch_thread = std::jthread([s_serial, s_state](std::stop_token s_stop) {
    detail::LatestMailbox<kMaxTxFrameSize>::Frame s_control_frame;
    TxState::ReliableFrame s_reliable_frame;

    while (!s_stop.stop_requested()) {
      if (s_state->m_write_in_flight.load(std::memory_order_acquire)) {
        const auto s_epoch = s_state->m_wake_epoch.load(std::memory_order_acquire);
        std::unique_lock<std::mutex> s_lock(s_state->m_wake_mutex);
        s_state->m_wake_cv.wait(s_lock, [&] {
          return s_stop.stop_requested() ||
                 s_state->m_wake_epoch.load(std::memory_order_acquire) != s_epoch;
        });
        continue;
      }

      std::span<const std::uint8_t> s_data;
      TxCompletion s_completion;
      if (s_state->tryTakeReliable(s_reliable_frame)) {
        s_data = std::span(s_reliable_frame.m_data.data(), s_reliable_frame.m_size);
        s_completion = std::move(s_reliable_frame.m_completion);
      } else if (s_state->m_control_mailbox.try_take(s_control_frame)) {
        s_data = std::span(s_control_frame.m_data.data(), s_control_frame.m_size);
      } else {
        const auto s_epoch = s_state->m_wake_epoch.load(std::memory_order_acquire);
        std::unique_lock<std::mutex> s_lock(s_state->m_wake_mutex);
        s_state->m_wake_cv.wait(s_lock, [&] {
          return s_stop.stop_requested() ||
                 s_state->m_wake_epoch.load(std::memory_order_acquire) != s_epoch;
        });
        continue;
      }

      s_state->m_write_in_flight.store(true, std::memory_order_release);
      s_serial->async_write(s_data, [s_state, s_completion = std::move(s_completion)](
                                         const std::error_code &s_error,
                                         std::size_t) mutable {
        s_state->m_write_in_flight.store(false, std::memory_order_release);
        s_state->wake();
        if (s_completion) s_completion(completionStatus(s_error));
      });
    }
  });
}

AstrialUSBTransport::~AstrialUSBTransport() {
  if (m_tx_state) {
    m_tx_state->m_dispatch_thread.request_stop();
    m_tx_state->wake();
    if (m_tx_state->m_dispatch_thread.joinable()) {
      m_tx_state->m_dispatch_thread.join();
    }
  }
  if (m_serial) {
    m_serial->close();
  }
}

TxSubmitResult AstrialUSBTransport::submit(TxClass s_class,
                                           std::span<const std::uint8_t> s_data,
                                           TxCompletion s_completion) {
  if (!m_serial || !m_tx_state) {
    if (s_completion) {
      s_completion(TxCompletionStatus::Disconnected);
    }
    return TxSubmitResult::Disconnected;
  }

  if (s_data.size() > kMaxTxFrameSize) {
    if (s_completion) s_completion(TxCompletionStatus::IoError);
    return TxSubmitResult::QueueFull;
  }

  if (s_class == TxClass::ControlLatest) {
    // LatestMailbox is SPSC by design. Do not turn a simultaneous Arm/Gripper
    // control loop into a contended lock: reject that unsupported producer
    // topology instead of permitting a data race on the producer-owned slot.
    if (m_tx_state->m_control_producer_busy.test_and_set(std::memory_order_acquire)) {
      if (s_completion) s_completion(TxCompletionStatus::IoError);
      return TxSubmitResult::QueueFull;
    }
    if (!m_tx_state->m_control_mailbox.publish(s_data)) {
      m_tx_state->m_control_producer_busy.clear(std::memory_order_release);
      return TxSubmitResult::QueueFull;
    }
    m_tx_state->m_control_producer_busy.clear(std::memory_order_release);
    m_tx_state->wake();
    return TxSubmitResult::Accepted;
  }

  bool s_queue_full = false;
  {
    std::lock_guard<std::mutex> s_lock(m_tx_state->m_reliable_mutex);
    if (m_tx_state->m_reliable_queue.size() == kReliableQueueCapacity) {
      s_queue_full = true;
    } else {
      auto &s_frame = m_tx_state->m_reliable_queue.emplace_back();
      std::memcpy(s_frame.m_data.data(), s_data.data(), s_data.size());
      s_frame.m_size = s_data.size();
      s_frame.m_completion = std::move(s_completion);
    }
  }
  if (s_queue_full) {
    if (s_completion) s_completion(TxCompletionStatus::IoError);
    return TxSubmitResult::QueueFull;
  }
  m_tx_state->wake();
  return TxSubmitResult::Accepted;
}

void AstrialUSBTransport::setReceiveCallback(ReceiveFunctor s_callback,
                                             void *s_context) {
  m_recv_callback = s_callback;
  m_recv_context = s_context;

  if (m_recv_callback && m_serial) {
    m_serial->on_data([this](std::span<const std::uint8_t> s_data) {
      if (m_recv_callback) {
        m_recv_callback(m_recv_context, s_data.data(), s_data.size());
      }
    });
  }
}

void AstrialUSBTransport::poll() {
  // Astrial has its own internal ASIO io_context thread driving async I/O.
  // No polling needed on the host platform.
}

bool AstrialUSBTransport::isConnected() const { return m_serial != nullptr; }

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
