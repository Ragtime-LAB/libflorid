#ifndef FLORID_DETAIL_ARM_IMPL_HPP
#define FLORID_DETAIL_ARM_IMPL_HPP

#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/detail/Transport.hpp"
#include "florid/detail/TickProvider.hpp"

#include "fci_protocol/session/arm_control_session.hpp"
#include "fci_protocol/transport/byte_stream_transport.hpp"
#include "fci_protocol/arm/packets.hpp"
#include "fci_protocol/arm/device_info.hpp"
#include "fci_protocol/arm/constants.hpp"

#include "readerwriterqueue.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <semaphore>
#include <chrono>
#include <mutex>

namespace florid {

enum class ReconnectPolicy {
    kThrow,
    kWait,
};

class ArmImpl;

class ArmControl {
public:
    Duration firmwarePeriod() const;
    Duration stateAge() const;
    Duration estimatedLatency() const;
    bool isReconnecting() const;
    void finishMotion();
    void stopControl();

private:
    friend class ArmImpl;
    ArmImpl* m_impl{nullptr};
};

class ArmImpl {
public:
    using SendFunc = std::function<void(const std::uint8_t*, std::size_t)>;
    using Session = fci::session::ArmControlSession<detail::MonotonicTickProvider, SendFunc>;

    explicit ArmImpl(std::unique_ptr<Transport> s_transport);
    ~ArmImpl();

    ArmImpl(const ArmImpl&) = delete;
    ArmImpl& operator=(const ArmImpl&) = delete;

    // ── Receive pipeline (called from Transport callback / Astrial on_data) ──
    static void s_onPhysData(void* s_context, const std::uint8_t* s_data, std::size_t s_size);

    // ── Device info (fetched during construction) ──
    const fci::arm::DeviceInfo& getDeviceInfo() const { return m_device_info; }
    std::uint32_t firmwarePeriodUs() const { return m_fw_dt_us; }
    fci::arm::FirmwareType firmwareType() const { return static_cast<fci::arm::FirmwareType>(m_device_info.fw_type); }

    // ── Arm state access ──
    ArmState readOnce();

    // ── Connection state ──
    bool isConnected() const { return m_connected.load(); }

protected:
    virtual bool s_supportsCartesian() const { return true; }

private:
    void s_feedBytes(const std::uint8_t* s_data, std::size_t s_size);
    void s_fetchDeviceInfo();

    // ── Protocol session ──
    Session m_session;

    // ── Physical transport ──
    std::unique_ptr<Transport> m_transport;

    // ── SPSC queue for control thread ──
    moodycamel::ReaderWriterQueue<fci::arm::ArmStatus> m_rx_queue{64};
    std::counting_semaphore<64> m_data_ready{0};
    std::uint32_t m_last_status_seq{0};

    // ── Cached DeviceInfo ──
    fci::arm::DeviceInfo m_device_info{};
    std::uint32_t m_fw_dt_us{2000};

    // ── Connection ──
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    ReconnectPolicy m_reconnect_policy{ReconnectPolicy::kThrow};
    std::chrono::milliseconds m_recv_timeout{50};

    // ── Control handle ──
    ArmControl m_arm_control;
    std::mutex m_control_mutex;
    std::atomic<bool> m_reconnecting{false};
    std::atomic<bool> m_stop_flag{false};

    // ── Timing ──
    double m_max_frequency_hz{500.0};

    friend class ArmControl;
};

} // namespace florid

#endif // FLORID_DETAIL_ARM_IMPL_HPP
