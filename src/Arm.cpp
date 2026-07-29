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

Gripper& Arm::gripper() {
    static Gripper s_gripper(*this);
    return s_gripper;
}

// ── Control loops ──

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

// ── State reading ──

ArmState Arm::readOnce() {
    return m_impl->readOnce();
}

// ── Active control ──

std::unique_ptr<ActiveControl<JointMIT>> Arm::startJointMITControl() {
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointMIT>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointMIT& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<JointPosVel>> Arm::startJointPosVelControl() {
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointPosVel>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointPosVel& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<JointVel>> Arm::startJointVelControl() {
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointVel>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointVel& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<JointPVT>> Arm::startJointPVTControl() {
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointPVT>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointPVT& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<CartesianPose>> Arm::startCartesianPoseControl() {
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<CartesianPose>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const CartesianPose& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<CartesianVelocities>> Arm::startCartesianVelocityControl() {
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<CartesianVelocities>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const CartesianVelocities& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

// ── Configuration ──

void Arm::home() { m_impl->home(); }
void Arm::enable() { m_impl->enable(); }
void Arm::drag() { m_impl->drag(); }
void Arm::disable() { m_impl->disable(); }
void Arm::setJointImpedance(const float (&s_K)[6]) { m_impl->setJointImpedance(s_K); }
void Arm::setCartesianImpedance(const float (&s_K)[6]) { m_impl->setCartesianImpedance(s_K); }
void Arm::setEEFrame(const float (&s_T)[16]) { m_impl->setEEFrame(s_T); }
void Arm::setLoad(float s_mass, const float (&s_com)[3], const float (&s_inertia)[9]) {
    m_impl->setLoad(s_mass, s_com, s_inertia);
}
void Arm::automaticErrorRecovery() { m_impl->automaticErrorRecovery(); }
void Arm::stop() { m_impl->stop(); }

// ── Motor register access ──

std::optional<float> Arm::readMotorRegister(std::uint8_t s_joint_id, fci::arm::MotorRegister s_rid) {
    return m_impl->readMotorRegister(s_joint_id, s_rid);
}
bool Arm::writeMotorRegister(std::uint8_t s_joint_id, fci::arm::MotorRegister s_rid, float s_value) {
    return m_impl->writeMotorRegister(s_joint_id, s_rid, s_value);
}
bool Arm::storeParameters(std::uint8_t s_joint_id) {
    return m_impl->storeParameters(s_joint_id);
}
bool Arm::setZeroPoint(std::uint8_t s_joint_id) {
    return m_impl->setZeroPoint(s_joint_id);
}

std::uint32_t Arm::firmwarePeriodUs() const { return m_impl->firmwarePeriodUs(); }
ReconnectPolicy Arm::reconnectPolicy() const { return m_impl->reconnectPolicy(); }
void Arm::setReconnectPolicy(ReconnectPolicy s_p) { m_impl->setReconnectPolicy(s_p); }
bool Arm::isConnected() const { return m_impl->isConnected(); }
const fci::arm::DeviceInfo& Arm::deviceInfo() const { return m_impl->getDeviceInfo(); }
const fci::arm::DeviceSettings& Arm::deviceSettings() const { return m_impl->getDeviceSettings(); }
bool Arm::setDeviceSettings(const fci::arm::DeviceSettings& s_settings) { return m_impl->setDeviceSettings(s_settings); }
fci::arm::ArmDiagnostics Arm::readDiagnostics() { return m_impl->readDiagnostics(); }

} // namespace florid
