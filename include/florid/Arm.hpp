#ifndef FLORID_ARM_HPP
#define FLORID_ARM_HPP

#include "florid/ArmControl.hpp"
#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/DeviceDiscovery.hpp"
#include "florid/DeviceTypes.hpp"
#include "florid/Errors.hpp"
#include "florid/core/ActiveControl.hpp"
#include "florid/Gripper.hpp"

#include <cstdint>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace florid {

class ArmImpl;
struct ArmConnectionResult;

enum class ArmConnectionError {
    kNone = 0,
    kEnumerationFailed,
    kInvalidSelector,
    kDeviceNotFound,
    kAmbiguous,
    kPermissionDenied,
    kDeviceBusy,
    kDeviceDisconnected,
    kIncompatibleDevice,
    kControlUnavailable,
    kTransportError,
    kProtocolError,
};

class Arm {
public:
    static constexpr std::size_t s_kNumJoints = 6;

    static std::unique_ptr<Arm> create(const std::string& s_uri);

    // Product-level USB Bulk entry points. connect() selects the only visible
    // compatible Florid device; explicit selectors never silently choose one
    // of several matches.
    [[nodiscard]] static ArmConnectionResult connect();
    [[nodiscard]] static ArmConnectionResult connect(
        const DeviceSelector& s_selector);
    [[nodiscard]] static ArmConnectionResult connect(
        const DeviceDescriptor& s_device);

    Arm(Arm&& s_other) noexcept;
    Arm& operator=(Arm&& s_other) noexcept;
    ~Arm();

    Arm(const Arm&) = delete;
    Arm& operator=(const Arm&) = delete;

    // ── Control loop (blocking, runs callback in internal thread) ──

    void control(std::function<JointMIT(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<JointPosVel(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<JointVel(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<JointPVT(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<CartesianPose(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<CartesianVelocities(const ArmState&, ArmControl&)> s_cb);

    // ── State reading ──

    ArmState readOnce();

    template <typename Callable>
    void read(Callable&& s_cb) {
        while (true) {
            auto s_state = readOnce();
            if (s_state.m_seq == 0) continue;
            if (!s_cb(s_state)) break;
        }
    }

    Gripper& gripper();

    // ── Active control (manual read/write loop, pybind-friendly) ──

    std::unique_ptr<ActiveControl<JointMIT>> startJointMITControl();
    std::unique_ptr<ActiveControl<JointPosVel>> startJointPosVelControl();
    std::unique_ptr<ActiveControl<JointVel>> startJointVelControl();
    std::unique_ptr<ActiveControl<JointPVT>> startJointPVTControl();
    std::unique_ptr<ActiveControl<CartesianPose>> startCartesianPoseControl();
    std::unique_ptr<ActiveControl<CartesianVelocities>> startCartesianVelocityControl();

    // ── Configuration ──

    void home();
    void enable();
    void drag();
    void disable();
    void automaticErrorRecovery();
    void stop();

    // ── Motor register access (joint_id: 1–7, 1–6 = arm joints, 7 = gripper) ──

    std::optional<float> readMotorRegister(std::uint8_t s_joint_id,
                                           MotorRegister s_rid);
    bool writeMotorRegister(std::uint8_t s_joint_id, MotorRegister s_rid,
                            float s_value);
    bool storeParameters(std::uint8_t s_joint_id);
    bool setZeroPoint(std::uint8_t s_joint_id);

    std::uint32_t firmwarePeriodUs() const;
    const DeviceInfo& deviceInfo() const;
    const DeviceSettings& deviceSettings() const;
    bool setCustomName(const std::string& s_custom_name);
    bool setDeviceSettings(const DeviceSettings& s_settings);
    ArmDiagnostics readDiagnostics();

    [[nodiscard]] ArmConnectionState connectionState() const noexcept;
    // Waits for USB reconnection and reacquires the control lease if it expired
    // while the cable was absent.
    [[nodiscard]] bool waitUntilReady(
        std::chrono::milliseconds s_timeout);

private:
    Arm() = default;
    std::shared_ptr<ArmImpl> m_impl;
    std::unique_ptr<Gripper> m_gripper;
    friend class Gripper;
};

struct ArmConnectionResult {
    std::unique_ptr<Arm> m_arm;
    std::optional<DeviceDescriptor> m_device;
    std::vector<DeviceDescriptor> m_candidates;
    ArmConnectionError m_error{ArmConnectionError::kNone};
    std::error_code m_system_error;
    std::string m_message;

    [[nodiscard]] explicit operator bool() const noexcept {
        return m_arm != nullptr && m_error == ArmConnectionError::kNone;
    }
};

[[nodiscard]] const char* armConnectionErrorMessage(
    ArmConnectionError s_error) noexcept;

} // namespace florid

#endif // FLORID_ARM_HPP
