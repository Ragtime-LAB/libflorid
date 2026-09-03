#include "florid/Gripper.hpp"
#include "florid/Arm.hpp"
#include "florid/detail/ArmImpl.hpp"

#include <utility>

namespace florid {

Gripper::Gripper(Arm& s_arm) {
    m_impl = s_arm.m_impl;
}

GripperState Gripper::readOnce() {
    if (!m_impl) return GripperState{};
    auto s_state = m_impl->readOnce();
    return GripperState{s_state.m_gripper_q, s_state.m_gripper_dq, s_state.m_gripper_tau};
}

void Gripper::control(std::function<JointMIT(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_gripperLoop(std::move(s_cb));
}

void Gripper::control(std::function<JointPosVel(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_gripperLoop(std::move(s_cb));
}

void Gripper::control(std::function<JointVel(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_gripperLoop(std::move(s_cb));
}

void Gripper::control(std::function<JointPVT(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_gripperLoop(std::move(s_cb));
}

std::unique_ptr<ActiveControl<JointMIT>> Gripper::startJointMITControl() {
    auto s_impl = m_impl;
    s_impl->s_prepareGripperControl<JointMIT>();
    return std::make_unique<ActiveControl<JointMIT>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointMIT& s_cmd) {
            s_impl->s_sendGripperCommand(s_cmd);
        });
}

} // namespace florid
