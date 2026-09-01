#ifndef FLORID_DETAIL_ARM_IMPL_HPP
#define FLORID_DETAIL_ARM_IMPL_HPP

#include "florid/ArmControl.hpp"
#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/DeviceTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"
#include "florid/detail/LatencyEstimator.hpp"
#include "florid/detail/Transport.hpp"

#ifdef FLORID_HAS_MPC
#include "florid/mpc/CartesianMPC.hpp"
#include "WillowMPCTraits.hpp"
#endif

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>

namespace florid {

class ArmImpl {
public:
    explicit ArmImpl(std::unique_ptr<Transport> s_transport);
    ~ArmImpl();

    ArmImpl(const ArmImpl&) = delete;
    ArmImpl& operator=(const ArmImpl&) = delete;

    static void s_onPhysData(void* s_context, const std::uint8_t* s_data,
                             std::size_t s_size) noexcept;

    const DeviceInfo& getDeviceInfo() const { return m_device_info; }
    const DeviceSettings& getDeviceSettings() const {
        return m_device_settings;
    }
    std::uint32_t firmwarePeriodUs() const { return m_fw_dt_us; }
    FirmwareType firmwareType() const { return m_device_info.m_firmware_type; }
    bool setDeviceSettings(const DeviceSettings& s_settings);

    ArmState readOnce();
    ArmDiagnostics readDiagnostics();
    ArmControl& controlHandle() { return m_arm_control; }

    template <typename Callback>
    void s_controlLoop(Callback s_cb) {
        using ReturnType = std::decay_t<decltype(s_cb(
            std::declval<const ArmState&>(), std::declval<ArmControl&>()))>;

        m_running = true;
        m_stop_flag = false;
        s_requestPcMode();
        s_ensureMode(s_controlModeFor<ReturnType>());

        while (m_running && !m_stop_flag) {
            m_data_ready.acquire();
            m_state_wake_pending.store(false, std::memory_order_release);
            if (!m_running || m_stop_flag) break;

            ArmState s_state{};
            if (!s_takeLatestState(s_state)) continue;
            auto s_command = s_cb(s_state, m_arm_control);
            s_sendCommand(s_command);
            if (s_command.m_motion_finished) break;
        }

        m_running = false;
    }

    template <typename Callback>
    void s_gripperLoop(Callback s_cb) {
        using ReturnType = std::decay_t<decltype(s_cb(
            std::declval<const ArmState&>(), std::declval<ArmControl&>()))>;

        m_running = true;
        m_stop_flag = false;
        s_requestPcMode();
        s_ensureGripperMode(s_controlModeFor<ReturnType>());

        while (m_running && !m_stop_flag) {
            m_data_ready.acquire();
            m_state_wake_pending.store(false, std::memory_order_release);
            if (!m_running || m_stop_flag) break;

            ArmState s_state{};
            if (!s_takeLatestState(s_state)) continue;
            auto s_command = s_cb(s_state, m_arm_control);
            s_sendGripperCommand(s_command);
            if (s_command.m_motion_finished) break;
        }

        m_running = false;
    }

    void home();
    void enable();
    void drag();
    void disable();
    void automaticErrorRecovery();
    void stop();

    std::optional<float> readMotorRegister(std::uint8_t s_joint_id,
                                           MotorRegister s_rid);
    bool writeMotorRegister(std::uint8_t s_joint_id, MotorRegister s_rid,
                            float s_value);
    bool storeParameters(std::uint8_t s_joint_id);
    bool setZeroPoint(std::uint8_t s_joint_id);

    bool isConnected() const { return m_connected.load(); }
    ReconnectPolicy reconnectPolicy() const { return m_reconnect_policy; }
    void setReconnectPolicy(ReconnectPolicy s_policy) {
        m_reconnect_policy = s_policy;
    }

    template <typename CommandType>
    void s_prepareControl() {
        s_requestPcMode();
        s_ensureMode(s_controlModeFor<CommandType>());
    }

    template <typename CommandType>
    void s_prepareGripperControl() {
        s_requestPcMode();
        s_ensureGripperMode(s_controlModeFor<CommandType>());
    }

    void s_sendCommand(const JointMIT& s_command);
    void s_sendCommand(const JointPosVel& s_command);
    void s_sendCommand(const JointVel& s_command);
    void s_sendCommand(const JointPVT& s_command);
    void s_sendCommand(const CartesianPose& s_command);
    void s_sendCommand(const CartesianVelocities& s_command);

    void s_sendGripperCommand(const JointMIT& s_command);
    void s_sendGripperCommand(const JointPosVel& s_command);
    void s_sendGripperCommand(const JointVel& s_command);
    void s_sendGripperCommand(const JointPVT& s_command);

protected:
    virtual bool s_supportsCartesian() const { return true; }
    virtual JointPVT s_convertCartesian(const CartesianPose& s_command,
                                        const ArmState& s_state);

private:
    static wl_sink_result_t s_wireSink(void* s_context,
                                        wl_io_token_t s_token,
                                        const std::uint8_t* s_data,
                                        std::size_t s_size) noexcept;
    static void s_onArmStatus(
        void* s_context,
        const detail::FciArmStatusSnapshot& s_status) noexcept;
    static void s_onDiagnostics(
        void* s_context, const ArmDiagnostics& s_diagnostics) noexcept;

    void s_feedBytes(const std::uint8_t* s_data, std::size_t s_size) noexcept;
    void s_fetchDeviceInfo();
    void s_fetchDeviceSettings();
    void s_ensureMode(detail::FciMotorControlMode s_mode);
    void s_ensureGripperMode(detail::FciMotorControlMode s_mode);
    void s_requestPcMode();
    void s_requireOperation(detail::FciSubmitResult s_submit,
                            std::chrono::milliseconds s_wait,
                            const char* s_operation);
    bool s_operationSucceeded(detail::FciSubmitResult s_submit,
                              std::chrono::milliseconds s_wait) noexcept;
    void s_requireCommand(detail::FciEndpointStatus s_status,
                          const char* s_operation);
    ArmState s_latestState() const;
    bool s_takeLatestState(ArmState& s_state) noexcept;

    static bool s_validJointId(std::uint8_t s_joint_id) noexcept;
    static std::uint8_t s_wireJointId(std::uint8_t s_joint_id) noexcept;

    template <typename CommandType>
    static constexpr detail::FciMotorControlMode s_controlModeFor() {
        if constexpr (std::is_same_v<CommandType, JointPosVel>) {
            return detail::FciMotorControlMode::kPositionVelocity;
        } else if constexpr (std::is_same_v<CommandType, JointVel>) {
            return detail::FciMotorControlMode::kVelocity;
        } else if constexpr (std::is_same_v<CommandType, JointPVT>) {
            return detail::FciMotorControlMode::kPvt;
        } else if constexpr (std::is_same_v<CommandType, CartesianPose>) {
#ifdef FLORID_HAS_MPC
            return detail::FciMotorControlMode::kPvt;
#else
            return detail::FciMotorControlMode::kMit;
#endif
        } else {
            return detail::FciMotorControlMode::kMit;
        }
    }

    // Transport outlives the endpoint. The destructor first detaches receive,
    // then stops the endpoint owner thread before either member is destroyed.
    std::unique_ptr<Transport> m_transport;
    detail::FciWirelinkEndpoint m_endpoint;

    std::counting_semaphore<65536> m_data_ready{0};
    std::atomic<bool> m_state_wake_pending{false};

    mutable std::mutex m_snapshot_mutex;
    ArmState m_latest_state{};
    std::uint64_t m_latest_state_generation{};
    std::uint64_t m_consumed_state_generation{};
    ArmDiagnostics m_last_diagnostics{};

    DeviceInfo m_device_info{};
    DeviceSettings m_device_settings{};
    std::uint32_t m_fw_dt_us{2000};

    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    ReconnectPolicy m_reconnect_policy{ReconnectPolicy::kThrow};

    ArmControl m_arm_control;
#ifdef FLORID_HAS_MPC
    std::unique_ptr<CartesianMPCSolver<WillowMPCTraits>> m_mpc;
#endif
    std::mutex m_control_mutex;
    std::atomic<bool> m_reconnecting{false};
    std::atomic<bool> m_stop_flag{false};
    std::optional<detail::FciMotorControlMode> m_current_mode;
    std::optional<detail::FciMotorControlMode> m_current_gripper_mode;
    mutable std::mutex m_latency_mutex;
    detail::LatencyEstimator m_latency;

    friend class ArmControl;
};

} // namespace florid

#endif // FLORID_DETAIL_ARM_IMPL_HPP
