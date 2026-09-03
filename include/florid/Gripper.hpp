#ifndef FLORID_GRIPPER_HPP
#define FLORID_GRIPPER_HPP

#include "florid/ArmControl.hpp"
#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/GripperState.hpp"
#include "florid/core/ActiveControl.hpp"

#include <functional>
#include <memory>

namespace florid {

class Arm;
class ArmImpl;

class Gripper {
public:
    explicit Gripper(Arm& s_arm);
    ~Gripper() = default;

    Gripper(const Gripper&) = delete;
    Gripper& operator=(const Gripper&) = delete;

    // ── Control loop ──

    void control(std::function<JointMIT(const ArmState&, ArmControl&)> s_cb);
    void control(std::function<JointPosVel(const ArmState&, ArmControl&)> s_cb);
    void control(std::function<JointVel(const ArmState&, ArmControl&)> s_cb);
    void control(std::function<JointPVT(const ArmState&, ArmControl&)> s_cb);

    // ── Active control (polling) ──

    std::unique_ptr<ActiveControl<JointMIT>> startJointMITControl();

    // ── State ──

    GripperState readOnce();

private:
    std::shared_ptr<ArmImpl> m_impl;
};

} // namespace florid

#endif // FLORID_GRIPPER_HPP
