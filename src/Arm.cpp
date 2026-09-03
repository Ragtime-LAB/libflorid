#include "florid/Arm.hpp"
#include "florid/Exceptions.hpp"
#include "florid/UsbDiscovery.hpp"
#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/AstrialBulkTransport.hpp"
#include "florid/detail/AstrialUSBTransport.hpp"
#include "florid/detail/UdpTransport.hpp"

#include <astrial/Usb.hpp>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <thread>

namespace florid {

namespace {

ArmConnectionError s_connectionError(DeviceDiscoveryError s_error) {
    switch (s_error) {
        case DeviceDiscoveryError::kNone:
            return ArmConnectionError::kNone;
        case DeviceDiscoveryError::kEnumerationFailed:
            return ArmConnectionError::kEnumerationFailed;
        case DeviceDiscoveryError::kInvalidSelector:
            return ArmConnectionError::kInvalidSelector;
        case DeviceDiscoveryError::kDeviceNotFound:
        case DeviceDiscoveryError::kTimeout:
            return ArmConnectionError::kDeviceNotFound;
        case DeviceDiscoveryError::kAmbiguous:
            return ArmConnectionError::kAmbiguous;
    }
    return ArmConnectionError::kProtocolError;
}

ArmConnectionError s_connectionError(const std::error_code& s_error) {
    if (s_error == make_error_code(UsbError::PermissionDenied)) {
        return ArmConnectionError::kPermissionDenied;
    }
    if (s_error == make_error_code(UsbError::InterfaceBusy)) {
        return ArmConnectionError::kDeviceBusy;
    }
    if (s_error == make_error_code(UsbError::DeviceDisconnected) ||
        s_error == make_error_code(UsbError::DeviceNotFound)) {
        return ArmConnectionError::kDeviceDisconnected;
    }
    return ArmConnectionError::kTransportError;
}

} // namespace

ArmConnectionResult Arm::connect() {
    return connect(DeviceSelector{});
}

ArmConnectionResult Arm::connect(const DeviceSelector& s_selector) {
    ArmConnectionResult s_result;
    const bool s_probe = s_selector.m_custom_name.has_value() ||
                         s_selector.m_firmware_type.has_value();
    const auto s_discovery = discoverDevices({.m_probe = s_probe});
    if (!s_discovery) {
        s_result.m_error = s_connectionError(s_discovery.m_error);
        s_result.m_system_error = s_discovery.m_system_error;
        s_result.m_message = deviceDiscoveryErrorMessage(s_discovery.m_error);
        return s_result;
    }

    auto s_selection = selectDevice(s_discovery.m_devices, s_selector);
    if (!s_selection) {
        s_result.m_error = s_connectionError(s_selection.m_error);
        s_result.m_candidates = std::move(s_selection.m_candidates);
        s_result.m_message =
            deviceDiscoveryErrorMessage(s_selection.m_error);
        return s_result;
    }
    return connect(*s_selection.m_device);
}

ArmConnectionResult Arm::connect(const DeviceDescriptor& s_device) {
    ArmConnectionResult s_result;
    s_result.m_device = s_device;
    if (s_device.m_access == DeviceAccessStatus::kPermissionDenied) {
        s_result.m_error = ArmConnectionError::kPermissionDenied;
    } else if (s_device.m_access == DeviceAccessStatus::kBusy) {
        s_result.m_error = ArmConnectionError::kDeviceBusy;
    } else if (s_device.m_access == DeviceAccessStatus::kDisconnected) {
        s_result.m_error = ArmConnectionError::kDeviceDisconnected;
    } else if (s_device.m_compatibility != DeviceCompatibility::kUnknown &&
               s_device.m_compatibility !=
                   DeviceCompatibility::kCompatible) {
        s_result.m_error = ArmConnectionError::kIncompatibleDevice;
    }
    if (s_result.m_error != ArmConnectionError::kNone) {
        s_result.m_system_error = s_device.m_system_error;
        s_result.m_message = s_device.m_error_message.empty()
                                 ? armConnectionErrorMessage(s_result.m_error)
                                 : s_device.m_error_message;
        return s_result;
    }

    try {
        auto s_transport = std::make_unique<AstrialBulkTransport>(
            s_device.m_usb.m_vendor_id, s_device.m_usb.m_product_id,
            s_device.m_usb.m_serial_number, s_device.m_usb.m_port_path);
        auto s_impl = std::make_shared<ArmImpl>(
            std::move(s_transport), s_device.m_usb.m_serial_number);
        auto s_arm = std::unique_ptr<Arm>(new Arm());
        s_arm->m_impl = std::move(s_impl);
        s_result.m_arm = std::move(s_arm);
        return s_result;
    } catch (const IncompatibleVersionException& s_error) {
        s_result.m_error = ArmConnectionError::kIncompatibleDevice;
        s_result.m_message = s_error.what();
    } catch (const NetworkException& s_error) {
        s_result.m_system_error = s_error.systemError();
        s_result.m_error = s_result.m_system_error
                               ? s_connectionError(s_result.m_system_error)
                               : ArmConnectionError::kTransportError;
        s_result.m_message = s_error.what();
    } catch (const CommandException& s_error) {
        s_result.m_error = ArmConnectionError::kControlUnavailable;
        s_result.m_message = s_error.what();
    } catch (const std::exception& s_error) {
        s_result.m_error = ArmConnectionError::kProtocolError;
        s_result.m_message = s_error.what();
    }
    return s_result;
}

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
            DeviceDescriptor s_device;
            s_device.m_usb = *s_selection.m_device;
            s_device.m_display_name = s_device.m_usb.m_display_name;
            auto s_connected = connect(s_device);
            if (!s_connected) {
                std::fprintf(stderr, "Arm::create failed: %s\n",
                             s_connected.m_message.c_str());
            }
            return std::move(s_connected.m_arm);
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

const char* armConnectionErrorMessage(ArmConnectionError s_error) noexcept {
    switch (s_error) {
        case ArmConnectionError::kNone: return "no error";
        case ArmConnectionError::kEnumerationFailed:
            return "USB enumeration failed";
        case ArmConnectionError::kInvalidSelector:
            return "invalid device selector";
        case ArmConnectionError::kDeviceNotFound: return "device not found";
        case ArmConnectionError::kAmbiguous:
            return "selector matches multiple devices";
        case ArmConnectionError::kPermissionDenied:
            return "USB device permission denied";
        case ArmConnectionError::kDeviceBusy:
            return "USB interface is busy";
        case ArmConnectionError::kDeviceDisconnected:
            return "USB device disconnected";
        case ArmConnectionError::kIncompatibleDevice:
            return "incompatible device";
        case ArmConnectionError::kControlUnavailable:
            return "device control is unavailable";
        case ArmConnectionError::kTransportError:
            return "transport connection failed";
        case ArmConnectionError::kProtocolError:
            return "protocol connection failed";
    }
    return "unknown connection error";
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

ArmConnectionState Arm::connectionState() const noexcept {
    return m_impl ? m_impl->connectionState() : ArmConnectionState::kClosed;
}

bool Arm::waitUntilReady(std::chrono::milliseconds s_timeout) {
    return m_impl && m_impl->waitUntilReady(s_timeout);
}

} // namespace florid
