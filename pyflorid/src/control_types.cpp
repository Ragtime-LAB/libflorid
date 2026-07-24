#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "florid/ControlTypes.hpp"

#include <cstring>

namespace py = pybind11;

static py::array_t<float> s_arr6(const float* s_p) { return py::array_t<float>(6, s_p); }
static py::array_t<float> s_arr16(const float* s_p) { return py::array_t<float>(16, s_p); }

static void s_set_arr(float* s_dst, py::array s_v, int s_n) {
    std::memcpy(s_dst, s_v.cast<py::array_t<float, py::array::c_style>>().data(), s_n * sizeof(float));
}

void bind_control_types(py::module_& m) {
    // ── JointMIT ──
    py::class_<florid::JointMIT>(m, "JointMIT")
        .def(py::init<>())
        .def_property("q",  [](florid::JointMIT& s) { return s_arr6(s.m_q); },
                           [](florid::JointMIT& s, py::array s_v) { s_set_arr(s.m_q, s_v, 6); })
        .def_property("dq", [](florid::JointMIT& s) { return s_arr6(s.m_dq); },
                           [](florid::JointMIT& s, py::array s_v) { s_set_arr(s.m_dq, s_v, 6); })
        .def_property("tau",[](florid::JointMIT& s) { return s_arr6(s.m_tau); },
                           [](florid::JointMIT& s, py::array s_v) { s_set_arr(s.m_tau, s_v, 6); })
        .def_property("kp", [](florid::JointMIT& s) { return s_arr6(s.m_kp); },
                           [](florid::JointMIT& s, py::array s_v) { s_set_arr(s.m_kp, s_v, 6); })
        .def_property("kd", [](florid::JointMIT& s) { return s_arr6(s.m_kd); },
                           [](florid::JointMIT& s, py::array s_v) { s_set_arr(s.m_kd, s_v, 6); })
        .def_readwrite("firmware_gravity", &florid::JointMIT::m_firmware_gravity)
        .def_readwrite("motion_finished",  &florid::JointMIT::m_motion_finished);

    // ── JointPosVel ──
    py::class_<florid::JointPosVel>(m, "JointPosVel")
        .def(py::init<>())
        .def_property("q",  [](florid::JointPosVel& s) { return s_arr6(s.m_q); },
                           [](florid::JointPosVel& s, py::array s_v) { s_set_arr(s.m_q, s_v, 6); })
        .def_property("dq", [](florid::JointPosVel& s) { return s_arr6(s.m_dq); },
                           [](florid::JointPosVel& s, py::array s_v) { s_set_arr(s.m_dq, s_v, 6); })
        .def_readwrite("motion_finished", &florid::JointPosVel::m_motion_finished);

    // ── JointVel ──
    py::class_<florid::JointVel>(m, "JointVel")
        .def(py::init<>())
        .def_property("dq", [](florid::JointVel& s) { return s_arr6(s.m_dq); },
                           [](florid::JointVel& s, py::array s_v) { s_set_arr(s.m_dq, s_v, 6); })
        .def_readwrite("motion_finished", &florid::JointVel::m_motion_finished);

    // ── JointPVT ──
    py::class_<florid::JointPVT>(m, "JointPVT")
        .def(py::init<>())
        .def_property("q",  [](florid::JointPVT& s) { return s_arr6(s.m_q); },
                           [](florid::JointPVT& s, py::array s_v) { s_set_arr(s.m_q, s_v, 6); })
        .def_property("dq_limit", [](florid::JointPVT& s) { return s_arr6(s.m_dq_limit); },
                           [](florid::JointPVT& s, py::array s_v) { s_set_arr(s.m_dq_limit, s_v, 6); })
        .def_property("current_limit_norm", [](florid::JointPVT& s) { return s_arr6(s.m_current_limit_norm); },
                           [](florid::JointPVT& s, py::array s_v) { s_set_arr(s.m_current_limit_norm, s_v, 6); })
        .def_readwrite("motion_finished", &florid::JointPVT::m_motion_finished);

    // ── CartesianPose ──
    py::class_<florid::CartesianPose>(m, "CartesianPose")
        .def(py::init<>())
        .def_property("T",  [](florid::CartesianPose& s) { return s_arr16(s.m_T); },
                           [](florid::CartesianPose& s, py::array s_v) { s_set_arr(s.m_T, s_v, 16); })
        .def_property("kp", [](florid::CartesianPose& s) { return s_arr6(s.m_kp); },
                           [](florid::CartesianPose& s, py::array s_v) { s_set_arr(s.m_kp, s_v, 6); })
        .def_property("kd", [](florid::CartesianPose& s) { return s_arr6(s.m_kd); },
                           [](florid::CartesianPose& s, py::array s_v) { s_set_arr(s.m_kd, s_v, 6); })
        .def_readwrite("motion_finished", &florid::CartesianPose::m_motion_finished);

    // ── CartesianVelocities ──
    py::class_<florid::CartesianVelocities>(m, "CartesianVelocities")
        .def(py::init<>())
        .def_property("v",  [](florid::CartesianVelocities& s) { return s_arr6(s.m_v); },
                           [](florid::CartesianVelocities& s, py::array s_v) { s_set_arr(s.m_v, s_v, 6); })
        .def_property("kp", [](florid::CartesianVelocities& s) { return s_arr6(s.m_kp); },
                           [](florid::CartesianVelocities& s, py::array s_v) { s_set_arr(s.m_kp, s_v, 6); })
        .def_property("kd", [](florid::CartesianVelocities& s) { return s_arr6(s.m_kd); },
                           [](florid::CartesianVelocities& s, py::array s_v) { s_set_arr(s.m_kd, s_v, 6); })
        .def_readwrite("motion_finished", &florid::CartesianVelocities::m_motion_finished);
}
