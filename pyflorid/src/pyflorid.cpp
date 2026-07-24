#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include "florid/Arm.hpp"
#include "florid/ArmState.hpp"
#include "florid/ArmControl.hpp"
#include "florid/ControlTypes.hpp"
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
    py::enum_<florid::ArmMode>(m, "ArmMode")
        .value("Init",    florid::ArmMode::Init)
        .value("Idle",    florid::ArmMode::Idle)
        .value("Running", florid::ArmMode::Running)
        .value("Fault",   florid::ArmMode::Fault)
        .value("EStop",   florid::ArmMode::EStop);

    py::enum_<florid::ReconnectPolicy>(m, "ReconnectPolicy")
        .value("Throw", florid::ReconnectPolicy::kThrow)
        .value("Wait",  florid::ReconnectPolicy::kWait);

    py::enum_<florid::ControllerMode>(m, "ControllerMode")
        .value("JointImpedance",    florid::ControllerMode::JointImpedance)
        .value("CartesianImpedance", florid::ControllerMode::CartesianImpedance);

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
        .def_readwrite("source_timestamp_us", &florid::ArmState::m_source_timestamp_us)
        .def_readwrite("mode",               &florid::ArmState::m_mode)
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
        .def("is_reconnecting", &florid::ArmControl::isReconnecting)
        .def("finish_motion",   &florid::ArmControl::finishMotion)
        .def("stop_control",    &florid::ArmControl::stopControl);

    // ── Arm ─────────────────────────────────────────
    py::class_<florid::Arm, std::shared_ptr<florid::Arm>> arm(m, "Arm");
    arm.def_static("create", &florid::Arm::create,
                py::arg("uri"), "Create arm from URI (e.g. 'usb:///dev/ttyACM1')");
    arm.def("home",              &florid::Arm::home);
    arm.def("read_once",         &florid::Arm::readOnce);
    arm.def("firmware_period_us", &florid::Arm::firmwarePeriodUs);
    arm.def("reconnect_policy",  &florid::Arm::reconnectPolicy);
    arm.def("set_reconnect_policy", &florid::Arm::setReconnectPolicy);
    arm.def("is_connected",      &florid::Arm::isConnected);
    arm.def("stop",              &florid::Arm::stop);
    arm.def("automatic_error_recovery", &florid::Arm::automaticErrorRecovery);
    arm.def("gripper", &florid::Arm::gripper, py::return_value_policy::reference);
    arm.def("start_joint_mit_control",               &florid::Arm::startJointMITControl);
    arm.def("start_joint_posvel_control",            &florid::Arm::startJointPosVelControl);
    arm.def("start_joint_vel_control",               &florid::Arm::startJointVelControl);
    arm.def("start_joint_pvt_control",               &florid::Arm::startJointPVTControl);
    arm.def("start_cartesian_pose_control",          &florid::Arm::startCartesianPoseControl);
    arm.def("start_cartesian_velocity_control",      &florid::Arm::startCartesianVelocityControl);

    // ── Sub-modules ─────────────────────────────────
    bind_control_types(m);
    bind_active_control(m);
    bind_model(m);
    bind_gripper(m);
}
