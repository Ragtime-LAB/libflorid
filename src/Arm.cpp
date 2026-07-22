#include "florid/Arm.hpp"
#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/AstrialUSBTransport.hpp"

#include <memory>
#include <thread>

namespace florid {

std::unique_ptr<Arm> Arm::create(const std::string& s_uri) {
    // "usb:///dev/ttyACM0" or "usb://COM3" or "tcp://host:port"
    std::unique_ptr<Transport> s_transport;

    if (s_uri.starts_with("usb://")) {
        std::string s_path = s_uri.substr(6); // strip "usb://"
        s_transport = std::make_unique<AstrialUSBTransport>(s_path);
    } else if (s_uri.starts_with("mock://")) {
        // Mock transport for testing — created externally via Arm(std::shared_ptr<ArmImpl>)
        return nullptr;
    } else {
        return nullptr;
    }

    auto s_impl = std::make_shared<ArmImpl>(std::move(s_transport));
    auto s_arm = std::unique_ptr<Arm>(new Arm());
    s_arm->m_impl = s_impl;
    return s_arm;
}

Arm::Arm(Arm&& s_other) noexcept : m_impl(std::move(s_other.m_impl)) {}

Arm& Arm::operator=(Arm&& s_other) noexcept {
    if (this != &s_other) m_impl = std::move(s_other.m_impl);
    return *this;
}

Arm::~Arm() = default;

// ── Control loops ──

void Arm::control(std::function<Torques(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_controlLoop(std::move(s_cb));
}

void Arm::control(std::function<JointMIT(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_controlLoop(std::move(s_cb));
}

void Arm::control(std::function<JointPosVel(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_controlLoop(std::move(s_cb));
}

void Arm::control(std::function<JointVel(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_controlLoop(std::move(s_cb));
}

void Arm::control(std::function<JointPVT(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_controlLoop(std::move(s_cb));
}

void Arm::control(std::function<CartesianPose(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_controlLoop(std::move(s_cb));
}

void Arm::control(std::function<CartesianVelocities(const ArmState&, ArmControl&)> s_cb) {
    m_impl->s_controlLoop(std::move(s_cb));
}

void Arm::control(
    std::function<Torques(const ArmState&, ArmControl&)> s_torque_cb,
    std::function<JointPosVel(const ArmState&, ArmControl&)> s_motion_cb) {
    m_impl->s_controlLoop([&](const ArmState& s_state, ArmControl& s_ctrl) {
        auto s_torque = s_torque_cb(s_state, s_ctrl);
        auto s_motion = s_motion_cb(s_state, s_ctrl);
        // Send motion command with torque feedforward
        // (torque is embedded via JointMIT, but for now just send torque)
        (void)s_motion;
        return s_torque;
    });
}

// ── State reading ──

ArmState Arm::readOnce() {
    return m_impl->readOnce();
}

// ── Configuration ──

void Arm::home() { m_impl->home(); }
void Arm::setJointImpedance(const float (&s_K)[6]) { m_impl->setJointImpedance(s_K); }
void Arm::setCartesianImpedance(const float (&s_K)[6]) { m_impl->setCartesianImpedance(s_K); }
void Arm::setEEFrame(const float (&s_T)[16]) { m_impl->setEEFrame(s_T); }
void Arm::setLoad(float s_mass, const float (&s_com)[3], const float (&s_inertia)[9]) {
    m_impl->setLoad(s_mass, s_com, s_inertia);
}
void Arm::automaticErrorRecovery() { m_impl->automaticErrorRecovery(); }
void Arm::stop() { m_impl->stop(); }

std::uint32_t Arm::firmwarePeriodUs() const { return m_impl->firmwarePeriodUs(); }
ReconnectPolicy Arm::reconnectPolicy() const { return m_impl->reconnectPolicy(); }
void Arm::setReconnectPolicy(ReconnectPolicy s_p) { m_impl->setReconnectPolicy(s_p); }
bool Arm::isConnected() const { return m_impl->isConnected(); }

} // namespace florid
