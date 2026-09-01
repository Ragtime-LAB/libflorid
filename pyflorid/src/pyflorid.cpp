#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include "florid/Arm.hpp"
#include "florid/ArmState.hpp"
#include "florid/ArmControl.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/DeviceTypes.hpp"
#include "florid/Duration.hpp"
#include "florid/Exceptions.hpp"

namespace py = pybind11;

// ── Sub-module bindings (declared in separate files) ──
void bind_control_types(py::module_& m);
void bind_active_control(py::module_& m);
void bind_model(py::module_& m);
void bind_gripper(py::module_& m);

PYBIND11_MODULE(_pyflorid, m) {
    m.doc() = "libflorid Python bindings — variable-frequency arm control SDK";

    // ── Exceptions ──────────────────────────────────
    py::register_exception<florid::Exception>(m, "Exception", PyExc_RuntimeError);
    py::register_exception<florid::NetworkException>(m, "NetworkException", PyExc_RuntimeError);
    py::register_exception<florid::ControlException>(m, "ControlException", PyExc_RuntimeError);
    py::register_exception<florid::CommandException>(m, "CommandException", PyExc_RuntimeError);
    py::register_exception<florid::InvalidOperationException>(m, "InvalidOperationException", PyExc_RuntimeError);
    py::register_exception<florid::RealtimeException>(m, "RealtimeException", PyExc_RuntimeError);

    // ── Enums ───────────────────────────────────────
    py::enum_<florid::ControllerMode>(m, "ControllerMode")
        .value("JointImpedance",    florid::ControllerMode::JointImpedance)
        .value("CartesianImpedance", florid::ControllerMode::CartesianImpedance);

    py::enum_<florid::FirmwareType>(m, "FirmwareType")
        .value("StandardArm", florid::FirmwareType::kStandardArm)
        .value("MobileArm", florid::FirmwareType::kMobileArm)
        .value("CobotArm", florid::FirmwareType::kCobotArm)
        .value("Unknown", florid::FirmwareType::kUnknown);

    py::enum_<florid::BusState>(m, "BusState")
        .value("ErrorActive", florid::BusState::kErrorActive)
        .value("ErrorWarning", florid::BusState::kErrorWarning)
        .value("ErrorPassive", florid::BusState::kErrorPassive)
        .value("BusOff", florid::BusState::kBusOff)
        .value("Stopped", florid::BusState::kStopped)
        .value("Unknown", florid::BusState::kUnknown);

    py::enum_<florid::MotorRegister>(m, "MotorRegister")
        .value("TorqueConstant",        florid::MotorRegister::TorqueConstant)
        .value("GearEfficiency",        florid::MotorRegister::GearEfficiency)
        .value("CurrentLoopBandwidth",  florid::MotorRegister::CurrentLoopBandwidth)
        .value("SpeedLoopKp",           florid::MotorRegister::SpeedLoopKp)
        .value("SpeedLoopKi",           florid::MotorRegister::SpeedLoopKi)
        .value("PositionLoopKp",        florid::MotorRegister::PositionLoopKp)
        .value("PositionLoopKi",        florid::MotorRegister::PositionLoopKi)
        .value("SpeedLoopDamping",      florid::MotorRegister::SpeedLoopDamping)
        .value("SpeedLoopFilterBW",     florid::MotorRegister::SpeedLoopFilterBW)
        .value("CurrentEnhanceFactor",  florid::MotorRegister::CurrentEnhanceFactor)
        .value("VelocityEnhanceFactor", florid::MotorRegister::VelocityEnhanceFactor)
        .value("VoltageUnder",          florid::MotorRegister::VoltageUnder)
        .value("VoltageOver",           florid::MotorRegister::VoltageOver)
        .value("TemperatureLimit",      florid::MotorRegister::TemperatureLimit)
        .value("OvercurrentLimit",      florid::MotorRegister::OvercurrentLimit)
        .value("Acceleration",          florid::MotorRegister::Acceleration)
        .value("Deceleration",          florid::MotorRegister::Deceleration)
        .value("MaxSpeed",              florid::MotorRegister::MaxSpeed)
        .value("PositionMax",           florid::MotorRegister::PositionMax)
        .value("VelocityMax",           florid::MotorRegister::VelocityMax)
        .value("TorqueMax",             florid::MotorRegister::TorqueMax)
        .value("DampingCoefficient",    florid::MotorRegister::DampingCoefficient)
        .value("Inertia",               florid::MotorRegister::Inertia)
        .value("HardwareVersion",       florid::MotorRegister::HardwareVersion)
        .value("SoftwareVersion",       florid::MotorRegister::SoftwareVersion)
        .value("PolePairs",             florid::MotorRegister::PolePairs)
        .value("PhaseResistance",       florid::MotorRegister::PhaseResistance)
        .value("PhaseInductance",       florid::MotorRegister::PhaseInductance)
        .value("FluxLinkage",           florid::MotorRegister::FluxLinkage)
        .value("GearRatio",             florid::MotorRegister::GearRatio)
        .value("SubVersion",            florid::MotorRegister::SubVersion)
        .value("MotorPosition",         florid::MotorRegister::MotorPosition)
        .value("OutputPosition",        florid::MotorRegister::OutputPosition);

    py::class_<florid::Version>(m, "Version")
        .def(py::init<>())
        .def_readwrite("major", &florid::Version::m_major)
        .def_readwrite("minor", &florid::Version::m_minor)
        .def_readwrite("patch", &florid::Version::m_patch);

    py::class_<florid::DeviceInfo>(m, "DeviceInfo")
        .def(py::init<>())
        .def_readonly("protocol_version", &florid::DeviceInfo::m_protocol_version)
        .def_readonly("fw_version", &florid::DeviceInfo::m_firmware_version)
        .def_readonly("board_name", &florid::DeviceInfo::m_board_name)
        .def_readonly("custom_name", &florid::DeviceInfo::m_custom_name)
        .def_readonly("fw_type", &florid::DeviceInfo::m_firmware_type);

    py::class_<florid::TorqueFoldParameters>(m, "TorqueFoldParameters")
        .def(py::init<>())
        .def_readwrite("continuous_torque", &florid::TorqueFoldParameters::m_continuous_torque)
        .def_readwrite("peak_torque", &florid::TorqueFoldParameters::m_peak_torque)
        .def_readwrite("thermal_capacity", &florid::TorqueFoldParameters::m_thermal_capacity)
        .def_readwrite("torque_ramp_rate", &florid::TorqueFoldParameters::m_torque_ramp_rate);

    py::class_<florid::JointLimits>(m, "JointLimits")
        .def(py::init<>())
        .def_readwrite("min", &florid::JointLimits::m_min)
        .def_readwrite("max", &florid::JointLimits::m_max);

    py::class_<florid::DeviceSettings>(m, "DeviceSettings")
        .def(py::init<>())
        .def_readwrite("firmware_period_us", &florid::DeviceSettings::m_firmware_period_us)
        .def_readwrite("gravity_scale", &florid::DeviceSettings::m_gravity_scale)
        .def_readwrite("torque_fold", &florid::DeviceSettings::m_torque_fold)
        .def_readwrite("joint_limits", &florid::DeviceSettings::m_joint_limits);

    // ── Duration ────────────────────────────────────
    py::class_<florid::Duration>(m, "Duration")
        .def(py::init<>())
        .def("to_sec",   &florid::Duration::toSec)
        .def("to_msec",  &florid::Duration::toMSec)
        .def("to_usec",  &florid::Duration::toUSec)
        .def("__repr__", [](const florid::Duration& s_d) {
            return std::to_string(s_d.toMSec()) + "ms";
        });

    // ── ArmState ────────────────────────────────────
    py::class_<florid::ArmState>(m, "ArmState")
        .def(py::init<>())
        .def_readwrite("time",               &florid::ArmState::m_time)
        .def_readwrite("seq",                &florid::ArmState::m_seq)
        .def_readwrite("mode",               &florid::ArmState::m_mode)
        .def_readwrite("source_timestamp_us", &florid::ArmState::m_source_timestamp_us)
        .def_readwrite("errors",             &florid::ArmState::m_errors)
        .def_property_readonly("q", [](const florid::ArmState& s) -> py::array_t<float> {
            return py::array_t<float>(6, s.m_q);
        })
        .def_property_readonly("dq", [](const florid::ArmState& s) -> py::array_t<float> {
            return py::array_t<float>(6, s.m_dq);
        })
        .def_property_readonly("tau", [](const florid::ArmState& s) -> py::array_t<float> {
            return py::array_t<float>(6, s.m_tau);
        })
        .def_property_readonly("base_gravity", [](const florid::ArmState& s) -> py::array_t<float> {
            return py::array_t<float>(3, s.m_base_gravity);
        })
        .def_property_readonly("O_T_EE", [](const florid::ArmState& s) -> py::array_t<float> {
            return py::array_t<float>(16, s.m_O_T_EE);
        })
        .def_property_readonly("F_ext", [](const florid::ArmState& s) -> py::array_t<float> {
            return py::array_t<float>(6, s.m_F_ext);
        })
        .def_readonly("gripper_q",   &florid::ArmState::m_gripper_q)
        .def_readonly("gripper_dq",  &florid::ArmState::m_gripper_dq)
        .def_readonly("gripper_tau", &florid::ArmState::m_gripper_tau);

    // ── ArmControl ──────────────────────────────────
    py::class_<florid::ArmControl>(m, "ArmControl")
        .def("firmware_period", &florid::ArmControl::firmwarePeriod)
        .def("state_age",       &florid::ArmControl::stateAge)
        .def("estimated_latency", &florid::ArmControl::estimatedLatency)
        .def("receive_jitter_us", &florid::ArmControl::receiveJitterUs)
        .def("receive_hz",      &florid::ArmControl::receiveHz)
        .def("finish_motion",   &florid::ArmControl::finishMotion)
        .def("stop_control",    &florid::ArmControl::stopControl);

    // ── Arm ─────────────────────────────────────────
    py::class_<florid::Arm, std::unique_ptr<florid::Arm>> arm(m, "Arm");
    arm.def_static("create", &florid::Arm::create,
                py::arg("uri"), "Create arm from URI (e.g. 'usb:///dev/ttyACM1' or 'udp://192.168.1.200:5080')");
    arm.def("home",              &florid::Arm::home);
    arm.def("enable",            &florid::Arm::enable);
    arm.def("drag",              &florid::Arm::drag);
    arm.def("disable",           &florid::Arm::disable);
    arm.def("read_once",         &florid::Arm::readOnce);
    arm.def("firmware_period_us", &florid::Arm::firmwarePeriodUs);
    arm.def("stop",              &florid::Arm::stop);
    arm.def("automatic_error_recovery", &florid::Arm::automaticErrorRecovery);
    arm.def("gripper", &florid::Arm::gripper, py::return_value_policy::reference);
    arm.def("start_joint_mit_control",               &florid::Arm::startJointMITControl);
    arm.def("start_joint_posvel_control",            &florid::Arm::startJointPosVelControl);
    arm.def("start_joint_vel_control",               &florid::Arm::startJointVelControl);
    arm.def("start_joint_pvt_control",               &florid::Arm::startJointPVTControl);
    arm.def("start_cartesian_pose_control",          &florid::Arm::startCartesianPoseControl);
    arm.def("start_cartesian_velocity_control",      &florid::Arm::startCartesianVelocityControl);
    arm.def("read_motor_register",   &florid::Arm::readMotorRegister);
    arm.def("write_motor_register",  &florid::Arm::writeMotorRegister);
    arm.def("store_parameters",       &florid::Arm::storeParameters);
    arm.def("set_zero_point",         &florid::Arm::setZeroPoint);
    arm.def("device_info",            &florid::Arm::deviceInfo,
            py::return_value_policy::reference_internal);
    arm.def("device_settings",        &florid::Arm::deviceSettings,
            py::return_value_policy::reference_internal);
    arm.def("set_device_settings",    &florid::Arm::setDeviceSettings);
    arm.def("read_diagnostics",      &florid::Arm::readDiagnostics);

    // ── Diagnostics structs ─────────────────────────
    py::class_<florid::JointDiagnostics>(m, "JointDiag")
        .def(py::init<>())
        .def_readonly("healthy", &florid::JointDiagnostics::m_healthy)
        .def_readonly("temp_c", &florid::JointDiagnostics::m_temperature_c);

    py::class_<florid::GripperDiagnostics>(m, "GripperDiag")
        .def(py::init<>())
        .def_readonly("healthy", &florid::GripperDiagnostics::m_healthy)
        .def_readonly("temp_c", &florid::GripperDiagnostics::m_temperature_c);

    py::class_<florid::ArmDiagnostics>(m, "ArmDiagnostics")
        .def(py::init<>())
        .def_readonly("uptime_s", &florid::ArmDiagnostics::m_uptime_s)
        .def_readonly("tick_count", &florid::ArmDiagnostics::m_tick_count)
        .def_readonly("mode_entry_ms", &florid::ArmDiagnostics::m_mode_entry_ms)
        .def_readonly("bus_healthy", &florid::ArmDiagnostics::m_bus_healthy)
        .def_readonly("bus_state", &florid::ArmDiagnostics::m_bus_state)
        .def_readonly("tx_err_count", &florid::ArmDiagnostics::m_tx_error_count)
        .def_readonly("rx_err_count", &florid::ArmDiagnostics::m_rx_error_count)
        .def_property_readonly("joints", [](const florid::ArmDiagnostics& s_d) {
            py::list l;
            for (const auto& s_joint : s_d.m_joints) l.append(s_joint);
            return l;
        })
        .def_readonly("gripper", &florid::ArmDiagnostics::m_gripper);

    // ── Sub-modules ─────────────────────────────────
    bind_control_types(m);
    bind_active_control(m);
    bind_model(m);
    bind_gripper(m);
}
