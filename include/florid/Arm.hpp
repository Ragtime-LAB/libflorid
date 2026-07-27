#ifndef FLORID_ARM_HPP
#define FLORID_ARM_HPP

#include "florid/ArmControl.hpp"
#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/Errors.hpp"
#include "florid/core/ActiveControl.hpp"
#include "florid/Gripper.hpp"

#include "fci_protocol/arm/constants.hpp"
#include "fci_protocol/arm/device_info.hpp"
#include "fci_protocol/arm/diagnostics.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace florid {

class ArmImpl;

class Arm {
public:
    static constexpr std::size_t s_kNumJoints = 6;

    static std::unique_ptr<Arm> create(const std::string& s_uri);

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
    void setJointImpedance(const float (&s_K)[6]);
    void setCartesianImpedance(const float (&s_K)[6]);
    void setEEFrame(const float (&s_T)[16]);
    void setLoad(float s_mass, const float (&s_com)[3], const float (&s_inertia)[9]);
    void automaticErrorRecovery();
    void stop();

    // ── Motor register access (joint_id: 1–7, 1–6 = arm joints, 7 = gripper) ──

    std::optional<float> readMotorRegister(std::uint8_t s_joint_id, fci::arm::MotorRegister s_rid);
    bool writeMotorRegister(std::uint8_t s_joint_id, fci::arm::MotorRegister s_rid, float s_value);
    bool storeParameters(std::uint8_t s_joint_id);
    bool setZeroPoint(std::uint8_t s_joint_id);

    std::uint32_t firmwarePeriodUs() const;
    ReconnectPolicy reconnectPolicy() const;
    void setReconnectPolicy(ReconnectPolicy s_p);
    bool isConnected() const;
    const fci::arm::DeviceInfo& deviceInfo() const;
    fci::arm::ArmDiagnostics readDiagnostics();

private:
    Arm() = default;
    std::shared_ptr<ArmImpl> m_impl;
    friend class Gripper;
};

} // namespace florid

#endif // FLORID_ARM_HPP
