#include "florid/Arm.hpp"
#include "florid/UsbDiscovery.hpp"
#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/AstrialBulkTransport.hpp"
#include "florid/detail/AstrialUSBTransport.hpp"
#include "florid/detail/UdpTransport.hpp"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <thread>

namespace florid {

std::unique_ptr<Arm> Arm::create(const std::string& s_uri) {
    // "usb://2fe3:574c" or "usb://2fe3:574c/SERIAL" (Vendor Bulk)
    // "serial:///dev/ttyACM0" or "serial://COM3" (legacy CDC/debug)
    // "udp://192.168.1.200:5080" — bind fixed local endpoint, device streams datagrams to it
    //
    // All connection failures are normalized to returning nullptr (with an
    // error printed to stderr) so callers only need a single `if (!arm)` check
    // regardless of which transport or URI path failed.
    try {
        std::unique_ptr<Transport> s_transport;

        if (s_uri.starts_with("usb://")) {
            const auto s_selection = resolveUsbBulkDevice(s_uri);
            if (!s_selection) {
                std::fprintf(
                    stderr, "Arm::create: %s for '%s'",
                    usbDiscoveryErrorMessage(s_selection.m_error),
                    s_uri.c_str());
                if (s_selection.m_error == UsbDiscoveryError::Ambiguous) {
                    std::fprintf(stderr, " (%zu matches; use an exact URI)",
                                 s_selection.m_match_count);
                } else if (s_selection.m_system_error) {
                    std::fprintf(stderr, ": %s",
                                 s_selection.m_system_error.message().c_str());
                }
                std::fputc('\n', stderr);
                return nullptr;
            }
            const auto& s_device = *s_selection.m_device;
            s_transport = std::make_unique<AstrialBulkTransport>(
                s_device.m_vendor_id, s_device.m_product_id,
                s_device.m_serial_number, s_device.m_port_path);
        } else if (s_uri.starts_with("serial://")) {
            std::string s_path = s_uri.substr(9);
            if (s_path.empty()) {
                std::fprintf(stderr, "Arm::create: empty serial path\n");
                return nullptr;
            }
            s_transport = std::make_unique<AstrialUSBTransport>(s_path);
        } else if (s_uri.starts_with("udp://")) {
            std::string s_host = s_uri.substr(6); // strip "udp://"
            std::uint16_t s_port = 5080;
            auto s_colon = s_host.rfind(':');
            if (s_colon != std::string::npos) {
                try {
                    s_port = static_cast<std::uint16_t>(std::stoul(s_host.substr(s_colon + 1)));
                } catch (...) {
                    std::fprintf(stderr, "Arm::create: invalid port in '%s'\n", s_uri.c_str());
                    return nullptr;
                }
                s_host = s_host.substr(0, s_colon);
            }
            if (s_host.empty()) {
                std::fprintf(stderr, "Arm::create: empty host in '%s'\n", s_uri.c_str());
                return nullptr;
            }
            s_transport = std::make_unique<UdpTransport>(s_host, s_port);
        } else {
            std::fprintf(stderr, "Arm::create: unknown URI scheme '%s'\n", s_uri.c_str());
            return nullptr;
        }

        auto s_impl = std::make_shared<ArmImpl>(std::move(s_transport));
        auto s_arm = std::unique_ptr<Arm>(new Arm());
        s_arm->m_impl = s_impl;
        return s_arm;
    } catch (const std::exception& s_e) {
        std::fprintf(stderr, "Arm::create failed: %s\n", s_e.what());
        return nullptr;
    }
}

Arm::Arm(Arm&& s_other) noexcept
    : m_impl(std::move(s_other.m_impl)),
      m_gripper(std::move(s_other.m_gripper)) {}

Arm& Arm::operator=(Arm&& s_other) noexcept {
    if (this != &s_other) {
        m_impl = std::move(s_other.m_impl);
        m_gripper = std::move(s_other.m_gripper);
    }
    return *this;
}

Arm::~Arm() = default;

Gripper& Arm::gripper() {
    if (!m_gripper) m_gripper = std::make_unique<Gripper>(*this);
    return *m_gripper;
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
    m_impl->s_prepareControl<JointMIT>();
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointMIT>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointMIT& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<JointPosVel>> Arm::startJointPosVelControl() {
    m_impl->s_prepareControl<JointPosVel>();
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointPosVel>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointPosVel& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<JointVel>> Arm::startJointVelControl() {
    m_impl->s_prepareControl<JointVel>();
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointVel>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointVel& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<JointPVT>> Arm::startJointPVTControl() {
    m_impl->s_prepareControl<JointPVT>();
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<JointPVT>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const JointPVT& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<CartesianPose>> Arm::startCartesianPoseControl() {
    m_impl->s_prepareControl<CartesianPose>();
    auto s_impl = m_impl;
    return std::make_unique<ActiveControl<CartesianPose>>(
        [s_impl] { return s_impl->readOnce(); },
        [s_impl](const CartesianPose& s_cmd) { s_impl->s_sendCommand(s_cmd); });
}

std::unique_ptr<ActiveControl<CartesianVelocities>> Arm::startCartesianVelocityControl() {
    m_impl->s_prepareControl<CartesianVelocities>();
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
void Arm::automaticErrorRecovery() { m_impl->automaticErrorRecovery(); }
void Arm::stop() { m_impl->stop(); }

// ── Motor register access ──

std::optional<float> Arm::readMotorRegister(std::uint8_t s_joint_id,
                                            MotorRegister s_rid) {
    return m_impl->readMotorRegister(s_joint_id, s_rid);
}
bool Arm::writeMotorRegister(std::uint8_t s_joint_id, MotorRegister s_rid,
                             float s_value) {
    return m_impl->writeMotorRegister(s_joint_id, s_rid, s_value);
}
bool Arm::storeParameters(std::uint8_t s_joint_id) {
    return m_impl->storeParameters(s_joint_id);
}
bool Arm::setZeroPoint(std::uint8_t s_joint_id) {
    return m_impl->setZeroPoint(s_joint_id);
}

std::uint32_t Arm::firmwarePeriodUs() const { return m_impl->firmwarePeriodUs(); }
const DeviceInfo& Arm::deviceInfo() const { return m_impl->getDeviceInfo(); }
const DeviceSettings& Arm::deviceSettings() const {
    return m_impl->getDeviceSettings();
}
bool Arm::setCustomName(const std::string& s_custom_name) {
    return m_impl->setCustomName(s_custom_name);
}
bool Arm::setDeviceSettings(const DeviceSettings& s_settings) {
    return m_impl->setDeviceSettings(s_settings);
}
ArmDiagnostics Arm::readDiagnostics() { return m_impl->readDiagnostics(); }

} // namespace florid
