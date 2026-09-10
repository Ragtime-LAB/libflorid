#ifndef FLORID_DETAIL_ARM_IMPL_HPP
#define FLORID_DETAIL_ARM_IMPL_HPP

#include "florid/ArmControl.hpp"
#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/DeviceTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/DeviceDiscovery.hpp"
#include "florid/Exceptions.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"
#include "florid/detail/ControlGeneration.hpp"
#include "florid/detail/LatencyEstimator.hpp"
#include "florid/detail/Transport.hpp"

#ifdef FLORID_HAS_MPC
#include "florid/mpc/CartesianMPC.hpp"
#include "florid/detail/MPCSession.hpp"
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
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace florid {

class ArmImpl {
public:
    explicit ArmImpl(std::unique_ptr<Transport> s_transport,
                     std::string s_expected_serial = {});
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
    bool setCustomName(std::string_view s_custom_name);
    bool setDeviceSettings(const DeviceSettings& s_settings);

    ArmState readOnce();
    ArmState latestState() const { return s_latestState(); }
#ifdef FLORID_HAS_MPC
    std::shared_ptr<detail::MPCSession> s_startMPC(const MPCControlConfig& s_config,
                                                  std::uint64_t* s_generation = nullptr);
#endif
    ArmDiagnostics readDiagnostics();
    [[nodiscard]] ArmConnectionState connectionState() const noexcept;
    [[nodiscard]] bool waitUntilReady(std::chrono::milliseconds s_timeout);
    ArmControl& controlHandle() { return m_arm_control; }
    detail::FciUpgradeClient& upgradeClient() { return m_endpoint.upgrade(); }
    detail::FciWirelinkEndpoint& firmwareEndpoint() { return m_endpoint; }
    bool firmwareUploadAllowed() const noexcept { return !m_running.load(std::memory_order_acquire); }

    template <typename Callback>
    void s_controlLoop(Callback s_cb) {
        using ReturnType = std::decay_t<decltype(s_cb(
            std::declval<const ArmState&>(), std::declval<ArmControl&>()))>;

        std::uint64_t s_generation;
#ifdef FLORID_HAS_MPC
        std::shared_ptr<detail::MPCSession> s_session;
        if constexpr (std::is_same_v<ReturnType, CartesianPose>) s_session = s_startMPC({}, &s_generation);
        else s_generation = s_prepareControl<ReturnType>();
        auto s_cleanup = [this, s_session, &s_generation](void*) {
            std::lock_guard s_lock(m_session_mutex);
            if (s_generation == m_generation.current()) m_running = false;
            if (s_session) s_session->stop();
        };
#else
        s_generation = s_prepareControl<ReturnType>();
        auto s_cleanup = [this, &s_generation](void*) {
            std::lock_guard s_lock(m_session_mutex);
            if (s_generation == m_generation.current()) m_running = false;
        };
#endif
        std::unique_ptr<void, decltype(s_cleanup)> s_guard(this, s_cleanup);

        while (m_running && !m_stop_flag && s_generation == m_generation.current()) {
            if (!m_data_ready.try_acquire_for(std::chrono::milliseconds(10))) {
#ifdef FLORID_HAS_MPC
                if (s_session) {
                    const auto s_status = s_session->status();
                    if (s_status.m_state == MPCControlState::kFault)
                        throw ControlException(mpcStopReasonMessage(s_status.m_stop_reason));
                    if (s_status.m_state == MPCControlState::kStopped) break;
                }
#endif
                continue;
            }
            m_state_wake_pending.store(false, std::memory_order_release);
            if (!m_running || m_stop_flag || s_generation != m_generation.current()) break;

            ArmState s_state{};
            if (!s_takeLatestState(s_state)) continue;
            auto s_command = s_cb(s_state, m_arm_control);
            if (!m_running || m_stop_flag || s_generation != m_generation.current()) break;
#ifdef FLORID_HAS_MPC
            if constexpr (std::is_same_v<ReturnType, CartesianPose>) s_session->write(s_command);
            else
#endif
            s_sendControlCommand(s_command, s_generation);
            if (s_command.m_motion_finished) break;
        }
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

    template <typename CommandType>
    std::uint64_t s_prepareControl() {
        std::lock_guard s_lock(m_session_mutex);
        return s_prepareControlLocked<CommandType>();
    }

    template <typename CommandType>
    void s_sendControlCommand(const CommandType& s_command, std::uint64_t s_generation) {
        m_generation.send(s_generation, [&] { s_sendCommand(s_command); });
    }

    template <typename CommandType>
    void s_prepareGripperControl() {
        if (!m_device_info.supports(s_gripperCapabilityFor<CommandType>())) {
            throw CommandException(
                "firmware does not advertise this gripper command capability");
        }
        s_requestPcMode();
        s_ensureGripperMode(s_controlModeFor<CommandType>());
    }

    void s_sendGripperCommand(const JointMIT& s_command);
    void s_sendGripperCommand(const JointPosVel& s_command);
    void s_sendGripperCommand(const JointVel& s_command);
    void s_sendGripperCommand(const JointPVT& s_command);

private:
    // Raw submissions are reachable only through the generation gate. MPC
    // targets use their bound session instead of the native command path.
    void s_sendCommand(const JointMIT& s_command);
    void s_sendCommand(const JointPosVel& s_command);
    void s_sendCommand(const JointVel& s_command);
    void s_sendCommand(const JointPVT& s_command);
#ifndef FLORID_HAS_MPC
    void s_sendCommand(const CartesianPose& s_command);
#endif
    void s_sendCommand(const CartesianVelocities& s_command);

    void s_stopControlLocked() noexcept;
    template <typename CommandType>
    std::uint64_t s_prepareControlLocked() {
        if (!m_device_info.supports(s_armCapabilityFor<CommandType>()))
            throw CommandException("firmware does not advertise this arm command capability");
        s_stopControlLocked();
        s_requestPcMode();
        s_ensureMode(s_controlModeFor<CommandType>());
        m_stop_flag = false;
        m_running = true;
        return m_generation.current();
    }
#ifdef FLORID_HAS_MPC
    detail::MPCMeasurement s_latestMeasurement() const;
#endif
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
    static constexpr CommandCapability s_armCapabilityFor() {
        if constexpr (std::is_same_v<CommandType, JointPosVel>) {
            return CommandCapability::kJointPositionVelocity;
        } else if constexpr (std::is_same_v<CommandType, JointVel>) {
            return CommandCapability::kJointVelocity;
        } else if constexpr (std::is_same_v<CommandType, JointPVT>) {
            return CommandCapability::kJointPvt;
        } else if constexpr (std::is_same_v<CommandType, CartesianPose>) {
#ifdef FLORID_HAS_MPC
            return CommandCapability::kJointPvt;
#else
            return CommandCapability::kCartesianPose;
#endif
        } else if constexpr (std::is_same_v<CommandType,
                                            CartesianVelocities>) {
            return CommandCapability::kCartesianVelocity;
        } else {
            return CommandCapability::kJointMit;
        }
    }

    template <typename CommandType>
    static constexpr CommandCapability s_gripperCapabilityFor() {
        if constexpr (std::is_same_v<CommandType, JointPosVel>) {
            return CommandCapability::kGripperPositionVelocity;
        } else if constexpr (std::is_same_v<CommandType, JointVel>) {
            return CommandCapability::kGripperVelocity;
        } else if constexpr (std::is_same_v<CommandType, JointPVT>) {
            return CommandCapability::kGripperPvt;
        } else {
            return CommandCapability::kGripperMit;
        }
    }

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
#ifdef FLORID_HAS_MPC
    // Endpoint callback is the only publisher. Each worker has its own consumer;
    // public readers serialize only with one another on m_snapshot_mutex.
    detail::LatestValue<detail::MPCMeasurement> m_planner_feedback, m_output_feedback;
    mutable detail::LatestValue<detail::MPCMeasurement> m_public_feedback;
    mutable detail::MPCMeasurement m_public_cache{};
    detail::MPCMeasurement m_received_feedback{}; // endpoint owner only
    std::atomic<bool> m_feedback_failed{false};
#else
    ArmState m_latest_state{};
    std::uint64_t m_latest_state_generation{};
#endif
    std::uint64_t m_consumed_state_generation{};
    std::mutex m_diagnostics_mutex;
    ArmDiagnostics m_last_diagnostics{};

    DeviceInfo m_device_info{};
    DeviceSettings m_device_settings{};
    std::uint32_t m_fw_dt_us{2000};

    std::atomic<bool> m_running{false};

    ArmControl m_arm_control;
#ifdef FLORID_HAS_MPC
    std::shared_ptr<detail::MPCSession> m_mpc;
#endif
    std::mutex m_session_mutex; // Cold lifecycle/RPC serialization only.
    detail::ControlGeneration m_generation;
    std::mutex m_control_mutex;
    std::atomic<bool> m_stop_flag{false};
    std::optional<detail::FciMotorControlMode> m_current_mode;
    std::optional<detail::FciMotorControlMode> m_current_gripper_mode;
    mutable std::mutex m_latency_mutex;
    detail::LatencyEstimator m_latency;

    friend class ArmControl;
};

} // namespace florid

#endif // FLORID_DETAIL_ARM_IMPL_HPP
