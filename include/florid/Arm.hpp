#ifndef FLORID_ARM_HPP
#define FLORID_ARM_HPP

#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/Errors.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace florid {

class ArmImpl;
class ArmControl;

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

    void control(std::function<Torques(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<JointMIT(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<JointPosVel(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<JointVel(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<JointPVT(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<CartesianPose(const ArmState&, ArmControl&)> s_cb);

    void control(std::function<CartesianVelocities(const ArmState&, ArmControl&)> s_cb);

    // ── Hybrid: torque control + motion generator ──

    void control(
        std::function<Torques(const ArmState&, ArmControl&)> s_torque_cb,
        std::function<JointPosVel(const ArmState&, ArmControl&)> s_motion_cb);

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

    // ── Configuration ──

    void setJointImpedance(const float (&s_K)[6]);
    void setCartesianImpedance(const float (&s_K)[6]);
    void setEEFrame(const float (&s_T)[16]);
    void setLoad(float s_mass, const float (&s_com)[3], const float (&s_inertia)[9]);
    void automaticErrorRecovery();
    void stop();

    std::uint32_t firmwarePeriodUs() const;
    ReconnectPolicy reconnectPolicy() const;
    void setReconnectPolicy(ReconnectPolicy s_p);
    bool isConnected() const;

private:
    Arm() = default;
    std::shared_ptr<ArmImpl> m_impl;
};

} // namespace florid

#endif // FLORID_ARM_HPP
