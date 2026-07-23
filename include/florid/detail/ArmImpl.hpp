#ifndef FLORID_DETAIL_ARM_IMPL_HPP
#define FLORID_DETAIL_ARM_IMPL_HPP

#include "florid/ArmControl.hpp"
#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/core/ArmCore.hpp"
#include "florid/core/traits.hpp"
#include "florid/detail/Transport.hpp"
#include "florid/detail/TickProvider.hpp"
#include "florid/detail/LatencyEstimator.hpp"

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
#include <thread>
namespace florid {

class ArmImpl {

public:
    using SendFunc = std::function<void(const std::uint8_t*, std::size_t)>;
    using Session = fci::session::ArmControlSession<detail::MonotonicTickProvider, SendFunc>;

    explicit ArmImpl(std::unique_ptr<Transport> s_transport);
    ~ArmImpl();

    ArmImpl(const ArmImpl&) = delete;
    ArmImpl& operator=(const ArmImpl&) = delete;

    // ── Receive pipeline ──
    static void s_onPhysData(void* s_context, const std::uint8_t* s_data, std::size_t s_size);

    // ── Device info ──
    const fci::arm::DeviceInfo& getDeviceInfo() const { return m_device_info; }
    std::uint32_t firmwarePeriodUs() const { return m_fw_dt_us; }
    fci::arm::FirmwareType firmwareType() const { return static_cast<fci::arm::FirmwareType>(m_device_info.fw_type); }

    // ── Arm state ──
    ArmState readOnce();
    ArmControl& controlHandle() { return m_arm_control; }

    // ── Control loop (template, called from Arm) ──
    template <typename Callback>
    void s_controlLoop(Callback s_cb) {
        using ReturnType = std::decay_t<decltype(s_cb(std::declval<const ArmState&>(),
                                                      std::declval<ArmControl&>()))>;

        m_running = true;
        m_stop_flag = false;

        // Auto-switch motor control mode if needed
        s_ensureMode(s_controlModeFor<ReturnType>());

        while (m_running && !m_stop_flag) {
            m_data_ready.acquire();

            fci::arm::ArmStatus s_raw;
            if (!m_rx_queue.try_dequeue(s_raw)) continue;

            ArmState s_state = s_convertStatus(s_raw);

            auto s_cmd = s_cb(s_state, m_arm_control);
            s_sendCommand(s_cmd);

            if (s_cmd.m_motion_finished) break;
        }

        m_running = false;
    }

    // ── Gripper control loop (no arm mode switch, packs externally) ──
    template <typename Callback, typename Packer>
    void s_gripperLoop(Callback s_cb, Packer s_packer) {
        using ReturnType = std::decay_t<decltype(s_cb(std::declval<const ArmState&>(),
                                                      std::declval<ArmControl&>()))>;

        m_running = true;
        m_stop_flag = false;

        while (m_running && !m_stop_flag) {
            m_data_ready.acquire();

            fci::arm::ArmStatus s_raw;
            if (!m_rx_queue.try_dequeue(s_raw)) continue;

            ArmState s_state = s_convertStatus(s_raw);

            auto s_cmd = s_cb(s_state, m_arm_control);
            s_packer(s_cmd);

            if (s_cmd.m_motion_finished) break;
        }

        m_running = false;
    }

    // ── Configuration ──
    void home();
    void setJointImpedance(const float (&s_K)[6]);
    void setCartesianImpedance(const float (&s_K)[6]);
    void setEEFrame(const float (&s_T)[16]);
    void setLoad(float s_mass, const float (&s_com)[3], const float (&s_inertia)[9]);
    void automaticErrorRecovery();
    void stop();

    // ── Connection ──
    bool isConnected() const { return m_connected.load(); }
    ReconnectPolicy reconnectPolicy() const { return m_reconnect_policy; }
    void setReconnectPolicy(ReconnectPolicy s_p) { m_reconnect_policy = s_p; }

    // ── Send (public, used by ActiveControl lambdas) ──
    template <typename CommandType>
    void s_sendCommand(const CommandType& s_cmd) {
        auto s_pkt = m_arm_core.s_pack(s_cmd);
        s_pkt.sdk_timestamp_us = detail::s_nowUs();
        m_session.notify(s_pkt);
    }

    void s_sendCommand(const CartesianPose& s_cmd);
    void s_sendCommand(const CartesianVelocities& s_cmd);

    // ── Generic notify (used by Gripper to send through shared session) ──
    template <typename ProtoPacket>
    void s_notify(const ProtoPacket& s_pkt) {
        m_session.notify(s_pkt);
    }

protected:
    virtual bool s_supportsCartesian() const { return true; }
    virtual JointPosVel s_convertCartesian(const CartesianPose& s_cmd, const ArmState& s_state);

    ArmCore m_arm_core;
    Session m_session;

private:
    void s_feedBytes(const std::uint8_t* s_data, std::size_t s_size);
    void s_fetchDeviceInfo();
    void s_ensureMode(fci::arm::MotorControlMode s_mode);

    ArmState s_convertStatus(const fci::arm::ArmStatus& s_raw);

    // ── Control mode helpers ──
    template <typename CmdType>
    static constexpr fci::arm::MotorControlMode s_controlModeFor() {
        if constexpr (std::is_same_v<CmdType, JointMIT>)               return fci::arm::MotorControlMode::MIT;
        if constexpr (std::is_same_v<CmdType, JointPosVel>)            return fci::arm::MotorControlMode::PosVel;
        if constexpr (std::is_same_v<CmdType, JointVel>)               return fci::arm::MotorControlMode::Vel;
        if constexpr (std::is_same_v<CmdType, JointPVT>)               return fci::arm::MotorControlMode::PVT;
        if constexpr (std::is_same_v<CmdType, CartesianPose>)          return fci::arm::MotorControlMode::MIT;
        if constexpr (std::is_same_v<CmdType, CartesianVelocities>)     return fci::arm::MotorControlMode::MIT;
        return fci::arm::MotorControlMode::MIT;
    }

    // ── Physical transport ──
    std::unique_ptr<Transport> m_transport;

    // ── SPSC queue ──
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

    // ── Control ──
    ArmControl m_arm_control;
    std::mutex m_control_mutex;
    std::atomic<bool> m_reconnecting{false};
    std::atomic<bool> m_stop_flag{false};
    double m_max_frequency_hz{500.0};
    fci::arm::MotorControlMode m_current_mode{static_cast<fci::arm::MotorControlMode>(0xFF)};
    detail::LatencyEstimator m_latency;

    friend class ArmControl;
};

} // namespace florid

#endif // FLORID_DETAIL_ARM_IMPL_HPP
