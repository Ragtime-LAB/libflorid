#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>
#include <cstring>

namespace py = pybind11;

namespace {

template <int N>
py::array_t<float> as_numpy(const float* src)
{
    auto arr = py::array_t<float>({N});
    std::memcpy(arr.mutable_data(), src, N * sizeof(float));
    return arr;
}

template <int N>
void from_numpy(const py::array_t<float>& arr, float* dst)
{
    if (arr.size() != N)
        throw std::runtime_error("Expected array of size " + std::to_string(N));
    std::memcpy(dst, arr.data(), N * sizeof(float));
}

py::array_t<float> mat44_as_numpy(const float* src)
{
    auto arr = py::array_t<float>({4, 4});
    std::memcpy(arr.mutable_data(), src, 64);
    return arr;
}

py::array_t<float> mat66_as_numpy(const float* src)
{
    auto arr = py::array_t<float>({6, 6});
    std::memcpy(arr.mutable_data(), src, 36 * sizeof(float));
    return arr;
}

} // anonymous

PYBIND11_MODULE(_pyflorid, m)
{
    m.doc() = "libflorid Python bindings — variable-frequency arm control";

    using namespace florid;
    using PMode = florid::protocol::SessionMode;

    // ── Enums ──────────────────────────────────────────────
    py::enum_<Frame>(m, "Frame")
        .value("kJoint1", Frame::kJoint1)
        .value("kJoint2", Frame::kJoint2)
        .value("kJoint3", Frame::kJoint3)
        .value("kJoint4", Frame::kJoint4)
        .value("kJoint5", Frame::kJoint5)
        .value("kJoint6", Frame::kJoint6)
        .value("kFlange", Frame::kFlange)
        .export_values();

    py::enum_<PMode>(m, "SessionMode")
        .value("Joint", PMode::Joint)
        .value("Point", PMode::Point)
        .value("Stop",  PMode::Stop)
        .export_values();

    // ── Duration ───────────────────────────────────────────
    py::class_<Duration>(m, "Duration")
        .def(py::init<uint64_t>())
        .def("to_sec",  &Duration::toSec)
        .def("to_msec", &Duration::toMSec);

    // ── ArmState ───────────────────────────────────────────
    py::class_<ArmState>(m, "ArmState")
        .def_readwrite("time",         &ArmState::time)
        .def_readwrite("mode",         &ArmState::mode)
        .def_readwrite("errors",       &ArmState::errors)
        .def_property_readonly("q",
            [](const ArmState& s) { return as_numpy<6>(s.q); })
        .def_property_readonly("dq",
            [](const ArmState& s) { return as_numpy<6>(s.dq); })
        .def_property_readonly("tau",
            [](const ArmState& s) { return as_numpy<6>(s.tau); })
        .def_property_readonly("tau_desired",
            [](const ArmState& s) { return as_numpy<6>(s.tau_desired); })
        .def_property_readonly("O_T_EE",
            [](const ArmState& s) { return mat44_as_numpy(s.O_T_EE); })
        .def_property_readonly("F_ext",
            [](const ArmState& s) { return as_numpy<6>(s.F_ext); })
        .def_property_readonly("base_gravity",
            [](const ArmState& s) { return as_numpy<3>(s.base_gravity); });

    // ── Control Types ──────────────────────────────────────
    py::class_<Torques>(m, "Torques")
        .def(py::init<>())
        .def_readwrite("motion_finished", &Torques::motion_finished)
        .def_property("tau",
            [](Torques& t) { return as_numpy<6>(t.tau); },
            [](Torques& t, py::array_t<float> a) { from_numpy<6>(a, t.tau); })
        .def_property("kp",
            [](Torques& t) { return as_numpy<6>(t.kp); },
            [](Torques& t, py::array_t<float> a) { from_numpy<6>(a, t.kp); })
        .def_property("kd",
            [](Torques& t) { return as_numpy<6>(t.kd); },
            [](Torques& t, py::array_t<float> a) { from_numpy<6>(a, t.kd); });

    py::class_<JointPositions>(m, "JointPositions")
        .def(py::init<>())
        .def_readwrite("motion_finished", &JointPositions::motion_finished)
        .def_property("q",
            [](JointPositions& j) { return as_numpy<6>(j.q); },
            [](JointPositions& j, py::array_t<float> a) { from_numpy<6>(a, j.q); })
        .def_property("kp",
            [](JointPositions& j) { return as_numpy<6>(j.kp); },
            [](JointPositions& j, py::array_t<float> a) { from_numpy<6>(a, j.kp); })
        .def_property("kd",
            [](JointPositions& j) { return as_numpy<6>(j.kd); },
            [](JointPositions& j, py::array_t<float> a) { from_numpy<6>(a, j.kd); });

    py::class_<JointVelocities>(m, "JointVelocities")
        .def(py::init<>())
        .def_readwrite("motion_finished", &JointVelocities::motion_finished)
        .def_property("dq",
            [](JointVelocities& j) { return as_numpy<6>(j.dq); },
            [](JointVelocities& j, py::array_t<float> a) { from_numpy<6>(a, j.dq); })
        .def_property("kp",
            [](JointVelocities& j) { return as_numpy<6>(j.kp); },
            [](JointVelocities& j, py::array_t<float> a) { from_numpy<6>(a, j.kp); })
        .def_property("kd",
            [](JointVelocities& j) { return as_numpy<6>(j.kd); },
            [](JointVelocities& j, py::array_t<float> a) { from_numpy<6>(a, j.kd); });

    py::class_<CartesianPose>(m, "CartesianPose")
        .def(py::init<>())
        .def_readwrite("motion_finished", &CartesianPose::motion_finished)
        .def_property("T",
            [](CartesianPose& p) { return mat44_as_numpy(p.T); },
            [](CartesianPose& p, py::array_t<float> a) {
                if (a.size() != 16) throw std::runtime_error("T must be 4x4 (16 elements)");
                std::memcpy(p.T, a.data(), 64);
            });

    py::class_<CartesianVelocities>(m, "CartesianVelocities")
        .def(py::init<>())
        .def_readwrite("motion_finished", &CartesianVelocities::motion_finished)
        .def_property("v",
            [](CartesianVelocities& c) { return as_numpy<6>(c.v); },
            [](CartesianVelocities& c, py::array_t<float> a) { from_numpy<6>(a, c.v); });

    // ── ArmControl ─────────────────────────────────────────
    py::class_<ArmControl>(m, "ArmControl")
        .def("finish_motion",  &ArmControl::finishMotion)
        .def("stop_control",   &ArmControl::stopControl)
        .def("is_stopped",     &ArmControl::isStopped)
        .def("is_finished",    &ArmControl::isFinished);

    // ── Arm ────────────────────────────────────────────────
    py::class_<Arm>(m, "Arm")
        .def(py::init<const char*, uint16_t, PMode>(),
             py::arg("ip"), py::arg("port") = 6040,
             py::arg("mode") = PMode::Joint)
        .def("read_once",  &Arm::readOnce)
        .def("update",     &Arm::update)
        .def("write_once", [](Arm& a, const Torques& cmd) { a.writeOnce(cmd); })
        .def("write_once", [](Arm& a, const JointPositions& cmd) { a.writeOnce(cmd); })
        .def("write_once", [](Arm& a, const JointVelocities& cmd) { a.writeOnce(cmd); })
        .def("write_once", [](Arm& a, const CartesianPose& cmd) { a.writeOnce(cmd); })
        .def("write_once", [](Arm& a, const CartesianVelocities& cmd) { a.writeOnce(cmd); })
        .def("set_max_frequency_hz", &Arm::setMaxFrequencyHz)
        .def("set_collision_behavior",
            [](Arm& a, py::array_t<float> lo, py::array_t<float> hi) {
                float l[6], h[6];
                from_numpy<6>(lo, l); from_numpy<6>(hi, h);
                a.setCollisionBehavior(l, h);
            })
        .def("set_watchdog_timeout",
            [](Arm& a, uint64_t ms) { a.setWatchdogTimeout(Duration(ms)); })
        .def("switch_control_mode", &Arm::switchControlMode)
        .def("stop", &Arm::stop)
        .def("automatic_error_recovery", &Arm::automaticErrorRecovery);

    // ── Model ──────────────────────────────────────────────
    using ConcreteModel = Model<PantheraTraits>;

    py::class_<ConcreteModel>(m, "Model")
        .def(py::init<>())
        .def("forward_kinematics",
            [](ConcreteModel& md, py::array_t<float> q) -> py::array_t<float> {
                float T[16]; md.forwardKinematics(q.data(), T);
                return mat44_as_numpy(T);
            })
        .def("mass",
            [](ConcreteModel& md, py::array_t<float> q) -> py::array_t<float> {
                float M[36]; md.mass(q.data(), M);
                return mat66_as_numpy(M);
            })
        .def("coriolis",
            [](ConcreteModel& md, py::array_t<float> q,
               py::array_t<float> dq) -> py::array_t<float> {
                float C[6]; md.coriolis(q.data(), dq.data(), C);
                return as_numpy<6>(C);
            })
        .def("gravity",
            [](ConcreteModel& md, py::array_t<float> q,
               py::array_t<float> gv) -> py::array_t<float> {
                float g[6]; md.gravity(q.data(), gv.data(), g);
                return as_numpy<6>(g);
            })
        .def("zero_jacobian",
            [](ConcreteModel& md, py::array_t<float> q) -> py::array_t<float> {
                float J[36]; md.zeroJacobian(q.data(), J);
                return mat66_as_numpy(J);
            })
        .def("body_jacobian",
            [](ConcreteModel& md, py::array_t<float> q) -> py::array_t<float> {
                float J[36]; md.bodyJacobian(q.data(), J);
                return mat66_as_numpy(J);
            })
        .def("pose",
            [](ConcreteModel& md, Frame frame, py::array_t<float> q) {
                float T[16]; md.pose(frame, q.data(), T);
                return mat44_as_numpy(T);
            });
}
