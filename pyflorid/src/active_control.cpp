#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>
#include <pybind11/stl.h>
#include <vector>

#include "florid/core/ActiveControl.hpp"
#include "florid/Gripper.hpp"
#ifdef FLORID_HAS_MPC
#include "florid/mpc/MPCControl.hpp"
#endif

namespace py = pybind11;

template <typename T>
void s_bind_active_control(py::module_& m, const char* s_name) {
    py::class_<florid::ActiveControl<T>>(m, s_name)
        .def("read_once",  &florid::ActiveControl<T>::readOnce)
        .def("write_once", &florid::ActiveControl<T>::writeOnce, py::arg("command"));
}

void bind_active_control(py::module_& m) {
    s_bind_active_control<florid::JointMIT>(m,               "ActiveJointMIT");
    s_bind_active_control<florid::JointPosVel>(m,            "ActiveJointPosVel");
    s_bind_active_control<florid::JointVel>(m,               "ActiveJointVel");
    s_bind_active_control<florid::JointPVT>(m,               "ActiveJointPVT");
#ifdef FLORID_HAS_MPC
    using Config = florid::MPCControlConfig;
    using Status = florid::MPCControlStatus;
    using Control = florid::CartesianMPCControl;
    using State = florid::MPCControlState;
    py::enum_<State>(m, "MPCControlState")
        .value("WaitingForTarget", State::kWaitingForTarget)
        .value("Planning", State::kPlanning).value("Running", State::kRunning)
        .value("Stopped", State::kStopped).value("Fault", State::kFault);
    py::class_<Config>(m, "MPCControlConfig")
        .def(py::init<>())
        .def_readwrite("current_limit_norm", &Config::current_limit_norm)
        .def_readwrite("velocity_excitation", &Config::velocity_excitation)
        .def_readwrite("max_joint_velocity", &Config::max_joint_velocity)
        .def_readwrite("max_joint_acceleration", &Config::max_joint_acceleration)
        .def_readwrite("max_tracking_error", &Config::max_tracking_error)
        .def_readwrite("state_timeout", &Config::state_timeout)
        .def_readwrite("target_timeout", &Config::target_timeout)
        .def_readwrite("plan_timeout", &Config::plan_timeout)
        .def_readwrite("output_lateness_limit", &Config::output_lateness_limit);
    py::class_<Status>(m, "MPCControlStatus")
        .def_readonly("state", &Status::m_state)
        .def_property_readonly("stop_reason", [](const Status& s) {
            return florid::mpcStopReasonMessage(s.m_stop_reason);
        })
        .def_readonly("solver_status", &Status::m_solver_status)
        .def_readonly("solves", &Status::m_solves)
        .def_readonly("solver_failures", &Status::m_solver_failures)
        .def_readonly("outputs", &Status::m_outputs)
        .def_readonly("plan_switches", &Status::m_plan_switches)
        .def_readonly("missed_planning_periods", &Status::m_missed_planning_periods)
        .def_readonly("missed_output_periods", &Status::m_missed_output_periods)
        .def_readonly("last_solve_ms", &Status::m_last_solve_ms)
        .def_readonly("max_solve_ms", &Status::m_max_solve_ms)
        .def_readonly("max_output_lateness_ms", &Status::m_max_output_lateness_ms)
        .def_readonly("state_age_ms", &Status::m_state_age_ms)
        .def_readonly("plan_age_ms", &Status::m_plan_age_ms)
        .def_property_readonly("reference_q", [](const Status& s) {
            return std::vector<float>(s.m_reference_q, s.m_reference_q + 6);
        })
        .def_property_readonly("reference_dq", [](const Status& s) {
            return std::vector<float>(s.m_reference_dq, s.m_reference_dq + 6);
        });
    py::class_<Control>(m, "ActiveCartesianPose")
        .def("read_once", &Control::readOnce)
        .def("write_once", &Control::writeOnce, py::arg("command"), py::call_guard<py::gil_scoped_release>())
        .def("status", &Control::status)
        .def("stop", &Control::stop, py::call_guard<py::gil_scoped_release>());
#else
    s_bind_active_control<florid::CartesianPose>(m,          "ActiveCartesianPose");
#endif
    s_bind_active_control<florid::CartesianVelocities>(m,    "ActiveCartesianVelocities");
}

void bind_gripper(py::module_& m) {
    py::class_<florid::Gripper>(m, "Gripper")
        .def("read_once", &florid::Gripper::readOnce)
        .def("start_joint_mit_control", [](florid::Gripper& s_g) {
            return s_g.startJointMITControl();
        });
}
