#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/core/ActiveControl.hpp>
#include <florid/core/types.hpp>
#include <florid/traits/PantheraTraits.hpp>
#include <florid/traits/WillowTraits.hpp>

#include <cstring>
#include <stdexcept>

namespace py = pybind11;

namespace {

template <typename T>
using numpy_in = py::array_t<T, py::array::c_style | py::array::forcecast>;

template <int N>
py::array_t<float> vector_to_numpy(const float* src)
{
    auto arr = py::array_t<float>(
        py::array::ShapeContainer{N},
        py::array::StridesContainer{static_cast<py::ssize_t>(sizeof(float))});
    auto* dst = arr.mutable_data();
    for (int i = 0; i < N; ++i)
        dst[i] = src[i];
    return arr;
}

template <int Rows, int Cols>
py::array_t<float> matrix_to_numpy(const float* src)
{
    auto arr = py::array_t<float>(
        py::array::ShapeContainer{Rows, Cols},
        py::array::StridesContainer{
            static_cast<py::ssize_t>(Cols * sizeof(float)),
            static_cast<py::ssize_t>(sizeof(float))});
    auto* dst = arr.mutable_data();
    for (int r = 0; r < Rows; ++r)
        for (int c = 0; c < Cols; ++c)
            dst[r * Cols + c] = src[r * Cols + c];
    return arr;
}

py::array_t<float> colmajor44_to_numpy(const float* src)
{
    auto arr = py::array_t<float>(
        py::array::ShapeContainer{4, 4},
        py::array::StridesContainer{
            static_cast<py::ssize_t>(4 * sizeof(float)),
            static_cast<py::ssize_t>(sizeof(float))});
    auto* dst = arr.mutable_data();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            dst[r * 4 + c] = src[c * 4 + r];
    return arr;
}

template <int N>
void vector_from_numpy(const numpy_in<float>& arr, float* dst, const char* name)
{
    if (arr.ndim() != 1 || arr.shape(0) != N)
        throw std::runtime_error(std::string(name) + " must have shape (" + std::to_string(N) + ",)");
    std::memcpy(dst, arr.data(), N * sizeof(float));
}

template <int Rows, int Cols>
void matrix_from_numpy(const numpy_in<float>& arr, float* dst, const char* name)
{
    if (arr.ndim() != 2 || arr.shape(0) != Rows || arr.shape(1) != Cols)
    {
        throw std::runtime_error(
            std::string(name) + " must have shape (" + std::to_string(Rows) + ", " + std::to_string(Cols) + ")");
    }
    std::memcpy(dst, arr.data(), Rows * Cols * sizeof(float));
}

void numpy_to_colmajor44(const numpy_in<float>& arr, float* dst, const char* name)
{
    if (arr.ndim() != 2 || arr.shape(0) != 4 || arr.shape(1) != 4)
        throw std::runtime_error(std::string(name) + " must have shape (4, 4)");
    const auto* src = arr.data();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            dst[c * 4 + r] = src[r * 4 + c];
}

template <typename ModelT>
void bind_model(py::module_& m, const char* name)
{
    py::class_<ModelT>(m, name)
        .def(py::init<>())
        .def("forward_kinematics",
             [](const ModelT& model, const numpy_in<float>& q) {
                 float q_buf[ModelT::kDOF];
                 float T[16];
                 vector_from_numpy<ModelT::kDOF>(q, q_buf, "q");
                 model.forwardKinematics(q_buf, T);
                 return colmajor44_to_numpy(T);
             })
        .def("pose",
             [](const ModelT& model, florid::Frame frame, const numpy_in<float>& q) {
                 float q_buf[ModelT::kDOF];
                 float T[16];
                 vector_from_numpy<ModelT::kDOF>(q, q_buf, "q");
                 model.pose(frame, q_buf, T);
                 return colmajor44_to_numpy(T);
             })
        .def("zero_jacobian",
             [](const ModelT& model, const numpy_in<float>& q) {
                 float q_buf[ModelT::kDOF];
                 float J[36];
                 vector_from_numpy<ModelT::kDOF>(q, q_buf, "q");
                 model.zeroJacobian(q_buf, J);
                 return matrix_to_numpy<6, 6>(J);
             })
        .def("body_jacobian",
             [](const ModelT& model, const numpy_in<float>& q) {
                 float q_buf[ModelT::kDOF];
                 float J[36];
                 vector_from_numpy<ModelT::kDOF>(q, q_buf, "q");
                 model.bodyJacobian(q_buf, J);
                 return matrix_to_numpy<6, 6>(J);
             })
        .def("gravity",
             [](const ModelT& model, const numpy_in<float>& q, const numpy_in<float>& g_vec) {
                 float q_buf[ModelT::kDOF];
                 float g_vec_buf[3];
                 float g[6];
                 vector_from_numpy<ModelT::kDOF>(q, q_buf, "q");
                 vector_from_numpy<3>(g_vec, g_vec_buf, "g_vec");
                 model.gravity(q_buf, g_vec_buf, g);
                 return vector_to_numpy<6>(g);
             })
        .def("mass",
             [](const ModelT& model, const numpy_in<float>& q) {
                 float q_buf[ModelT::kDOF];
                 float M[36];
                 vector_from_numpy<ModelT::kDOF>(q, q_buf, "q");
                 model.mass(q_buf, M);
                 return matrix_to_numpy<6, 6>(M);
             })
        .def("coriolis",
             [](const ModelT& model, const numpy_in<float>& q, const numpy_in<float>& dq) {
                 float q_buf[ModelT::kDOF];
                 float dq_buf[ModelT::kDOF];
                 float C[6];
                 vector_from_numpy<ModelT::kDOF>(q, q_buf, "q");
                 vector_from_numpy<ModelT::kDOF>(dq, dq_buf, "dq");
                 model.coriolis(q_buf, dq_buf, C);
                 return vector_to_numpy<6>(C);
             });
}

template <typename ControlT>
void bind_active_control(py::module_& m, const char* name)
{
    py::class_<florid::ActiveControl<ControlT>>(m, name)
        .def("read_once", &florid::ActiveControl<ControlT>::readOnce)
        .def("write_once", &florid::ActiveControl<ControlT>::writeOnce);
}

} // namespace

PYBIND11_MODULE(_pyflorid, m)
{
    m.doc() = "libflorid Python bindings";

    using florid::Arm;
    using florid::ArmControl;
    using florid::ArmMode;
    using florid::ArmState;
    using florid::CartesianPose;
    using florid::CartesianVelocities;
    using florid::Duration;
    using florid::Frame;
    using florid::JointPositions;
    using florid::JointVelocities;
    using florid::PantheraTraits;
    using florid::Torques;
    using florid::WillowTraits;
    using SessionMode = florid::protocol::SessionMode;
    using ControlMode = florid::protocol::ControlMode;

    py::enum_<ArmMode>(m, "ArmMode")
        .value("Init", ArmMode::Init)
        .value("Idle", ArmMode::Idle)
        .value("Running", ArmMode::Running)
        .value("Fault", ArmMode::Fault)
        .value("EStop", ArmMode::EStop);

    py::enum_<Frame>(m, "Frame")
        .value("Joint1", Frame::kJoint1)
        .value("Joint2", Frame::kJoint2)
        .value("Joint3", Frame::kJoint3)
        .value("Joint4", Frame::kJoint4)
        .value("Joint5", Frame::kJoint5)
        .value("Joint6", Frame::kJoint6)
        .value("Flange", Frame::kFlange)
        .value("EndEffector", Frame::kEndEffector);

    py::enum_<SessionMode>(m, "SessionMode")
        .value("Joint", SessionMode::Joint)
        .value("Point", SessionMode::Point)
        .value("Stop", SessionMode::Stop);

    py::enum_<ControlMode>(m, "ControlMode")
        .value("JointPosition", ControlMode::JointPosition)
        .value("JointVelocity", ControlMode::JointVelocity)
        .value("Torque", ControlMode::Torque)
        .value("CartesianPose", ControlMode::CartesianPose)
        .value("CartesianVelocity", ControlMode::CartesianVelocity);

    py::class_<Duration>(m, "Duration")
        .def(py::init<uint64_t>())
        .def("to_sec", &Duration::toSec)
        .def("to_msec", &Duration::toMSec);

    py::class_<ArmState>(m, "ArmState")
        .def_readwrite("time", &ArmState::time)
        .def_readwrite("mode", &ArmState::mode)
        .def_readwrite("errors", &ArmState::errors)
        .def_property_readonly("q", [](const ArmState& s) { return vector_to_numpy<6>(s.q); })
        .def_property_readonly("dq", [](const ArmState& s) { return vector_to_numpy<6>(s.dq); })
        .def_property_readonly("tau", [](const ArmState& s) { return vector_to_numpy<6>(s.tau); })
        .def_property_readonly("tau_desired", [](const ArmState& s) { return vector_to_numpy<6>(s.tau_desired); })
        .def_property_readonly("o_t_ee", [](const ArmState& s) { return colmajor44_to_numpy(s.O_T_EE); })
        .def_property_readonly("O_T_EE", [](const ArmState& s) { return colmajor44_to_numpy(s.O_T_EE); })
        .def_property_readonly("f_ext", [](const ArmState& s) { return vector_to_numpy<6>(s.F_ext); })
        .def_property_readonly("F_ext", [](const ArmState& s) { return vector_to_numpy<6>(s.F_ext); })
        .def_property_readonly("base_gravity", [](const ArmState& s) { return vector_to_numpy<3>(s.base_gravity); });

    py::class_<Torques>(m, "Torques")
        .def(py::init<>())
        .def_readwrite("motion_finished", &Torques::motion_finished)
        .def_property("tau",
                      [](const Torques& cmd) { return vector_to_numpy<6>(cmd.tau); },
                      [](Torques& cmd, const numpy_in<float>& tau) { vector_from_numpy<6>(tau, cmd.tau, "tau"); })
        .def_property("kp",
                      [](const Torques& cmd) { return vector_to_numpy<6>(cmd.kp); },
                      [](Torques& cmd, const numpy_in<float>& kp) { vector_from_numpy<6>(kp, cmd.kp, "kp"); })
        .def_property("kd",
                      [](const Torques& cmd) { return vector_to_numpy<6>(cmd.kd); },
                      [](Torques& cmd, const numpy_in<float>& kd) { vector_from_numpy<6>(kd, cmd.kd, "kd"); });

    py::class_<JointPositions>(m, "JointPositions")
        .def(py::init<>())
        .def_readwrite("motion_finished", &JointPositions::motion_finished)
        .def_property("q",
                      [](const JointPositions& cmd) { return vector_to_numpy<6>(cmd.q); },
                      [](JointPositions& cmd, const numpy_in<float>& q) { vector_from_numpy<6>(q, cmd.q, "q"); })
        .def_property("dq",
                      [](const JointPositions& cmd) { return vector_to_numpy<6>(cmd.dq); },
                      [](JointPositions& cmd, const numpy_in<float>& dq) { vector_from_numpy<6>(dq, cmd.dq, "dq"); })
        .def_property("kp",
                      [](const JointPositions& cmd) { return vector_to_numpy<6>(cmd.kp); },
                      [](JointPositions& cmd, const numpy_in<float>& kp) { vector_from_numpy<6>(kp, cmd.kp, "kp"); })
        .def_property("kd",
                      [](const JointPositions& cmd) { return vector_to_numpy<6>(cmd.kd); },
                      [](JointPositions& cmd, const numpy_in<float>& kd) { vector_from_numpy<6>(kd, cmd.kd, "kd"); });

    py::class_<JointVelocities>(m, "JointVelocities")
        .def(py::init<>())
        .def_readwrite("motion_finished", &JointVelocities::motion_finished)
        .def_property("dq",
                      [](const JointVelocities& cmd) { return vector_to_numpy<6>(cmd.dq); },
                      [](JointVelocities& cmd, const numpy_in<float>& dq) { vector_from_numpy<6>(dq, cmd.dq, "dq"); })
        .def_property("kp",
                      [](const JointVelocities& cmd) { return vector_to_numpy<6>(cmd.kp); },
                      [](JointVelocities& cmd, const numpy_in<float>& kp) { vector_from_numpy<6>(kp, cmd.kp, "kp"); })
        .def_property("kd",
                      [](const JointVelocities& cmd) { return vector_to_numpy<6>(cmd.kd); },
                      [](JointVelocities& cmd, const numpy_in<float>& kd) { vector_from_numpy<6>(kd, cmd.kd, "kd"); });

    py::class_<CartesianPose>(m, "CartesianPose")
        .def(py::init<>())
        .def_readwrite("motion_finished", &CartesianPose::motion_finished)
        .def_property("T",
                      [](const CartesianPose& cmd) { return colmajor44_to_numpy(cmd.T); },
                      [](CartesianPose& cmd, const numpy_in<float>& T) { numpy_to_colmajor44(T, cmd.T, "T"); })
        .def_property("kp",
                      [](const CartesianPose& cmd) { return vector_to_numpy<6>(cmd.kp); },
                      [](CartesianPose& cmd, const numpy_in<float>& kp) { vector_from_numpy<6>(kp, cmd.kp, "kp"); })
        .def_property("kd",
                      [](const CartesianPose& cmd) { return vector_to_numpy<6>(cmd.kd); },
                      [](CartesianPose& cmd, const numpy_in<float>& kd) { vector_from_numpy<6>(kd, cmd.kd, "kd"); });

    py::class_<CartesianVelocities>(m, "CartesianVelocities")
        .def(py::init<>())
        .def_readwrite("motion_finished", &CartesianVelocities::motion_finished)
        .def_property("v",
                      [](const CartesianVelocities& cmd) { return vector_to_numpy<6>(cmd.v); },
                      [](CartesianVelocities& cmd, const numpy_in<float>& v) { vector_from_numpy<6>(v, cmd.v, "v"); })
        .def_property("kp",
                      [](const CartesianVelocities& cmd) { return vector_to_numpy<6>(cmd.kp); },
                      [](CartesianVelocities& cmd, const numpy_in<float>& kp) { vector_from_numpy<6>(kp, cmd.kp, "kp"); })
        .def_property("kd",
                      [](const CartesianVelocities& cmd) { return vector_to_numpy<6>(cmd.kd); },
                      [](CartesianVelocities& cmd, const numpy_in<float>& kd) { vector_from_numpy<6>(kd, cmd.kd, "kd"); });

    py::class_<ArmControl>(m, "ArmControl")
        .def("finish_motion", &ArmControl::finishMotion)
        .def("stop_control", &ArmControl::stopControl)
        .def("is_stopped", &ArmControl::isStopped)
        .def("is_finished", &ArmControl::isFinished);

    bind_active_control<Torques>(m, "TorqueControl");
    bind_active_control<JointPositions>(m, "JointPositionControl");
    bind_active_control<JointVelocities>(m, "JointVelocityControl");
    bind_active_control<CartesianPose>(m, "CartesianPoseControl");
    bind_active_control<CartesianVelocities>(m, "CartesianVelocityControl");

    py::class_<Arm>(m, "Arm")
        .def(py::init<const char*, uint16_t, SessionMode>(),
             py::arg("ip"),
             py::arg("port") = 6040,
             py::arg("mode") = SessionMode::Joint)
        .def("read_once", &Arm::readOnce)
        .def("update", &Arm::update)
        .def("write_once", [](Arm& arm, const Torques& cmd) { arm.writeOnce(cmd); })
        .def("write_once", [](Arm& arm, const JointPositions& cmd) { arm.writeOnce(cmd); })
        .def("write_once", [](Arm& arm, const JointVelocities& cmd) { arm.writeOnce(cmd); })
        .def("write_once", [](Arm& arm, const CartesianPose& cmd) { arm.writeOnce(cmd); })
        .def("write_once", [](Arm& arm, const CartesianVelocities& cmd) { arm.writeOnce(cmd); })
        .def("set_max_frequency_hz", &Arm::setMaxFrequencyHz)
        .def("set_collision_behavior",
             [](Arm& arm, const numpy_in<float>& lower, const numpy_in<float>& upper) {
                 float lower_buf[6];
                 float upper_buf[6];
                 vector_from_numpy<6>(lower, lower_buf, "lower");
                 vector_from_numpy<6>(upper, upper_buf, "upper");
                 arm.setCollisionBehavior(lower_buf, upper_buf);
             })
        .def("set_cartesian_impedance",
             [](Arm& arm, const numpy_in<float>& stiffness) {
                 float stiffness_buf[6];
                 vector_from_numpy<6>(stiffness, stiffness_buf, "stiffness");
                 arm.setCartesianImpedance(stiffness_buf);
             })
        .def("set_k",
             [](Arm& arm, const numpy_in<float>& ee_t_k) {
                 float matrix[16];
                 if (ee_t_k.ndim() == 1 && ee_t_k.shape(0) == 16)
                 {
                     std::memcpy(matrix, ee_t_k.data(), sizeof(matrix));
                 }
                 else
                 {
                     numpy_to_colmajor44(ee_t_k, matrix, "ee_t_k");
                 }
                 arm.setK(matrix);
             })
        .def("set_ee",
             [](Arm& arm, const numpy_in<float>& ne_t_ee) {
                 float matrix[16];
                 if (ne_t_ee.ndim() == 1 && ne_t_ee.shape(0) == 16)
                 {
                     std::memcpy(matrix, ne_t_ee.data(), sizeof(matrix));
                 }
                 else
                 {
                     numpy_to_colmajor44(ne_t_ee, matrix, "ne_t_ee");
                 }
                 arm.setEE(matrix);
             })
        .def("set_load",
             [](Arm& arm, float mass, const numpy_in<float>& f_x_cload, const numpy_in<float>& load_inertia) {
                 float center[3];
                 float inertia[9];
                 vector_from_numpy<3>(f_x_cload, center, "f_x_cload");
                 if (load_inertia.ndim() == 1 && load_inertia.shape(0) == 9)
                 {
                     std::memcpy(inertia, load_inertia.data(), sizeof(inertia));
                 }
                 else
                 {
                     matrix_from_numpy<3, 3>(load_inertia, inertia, "load_inertia");
                 }
                 arm.setLoad(mass, center, inertia);
             })
        .def("set_watchdog_timeout", [](Arm& arm, uint64_t ms) { arm.setWatchdogTimeout(Duration(ms)); })
        .def("switch_control_mode", &Arm::switchControlMode)
        .def("stop", &Arm::stop)
        .def("automatic_error_recovery", &Arm::automaticErrorRecovery)
        .def("start_torque_control", &Arm::startTorqueControl)
        .def("start_joint_position_control", &Arm::startJointPositionControl)
        .def("start_joint_velocity_control", &Arm::startJointVelocityControl)
        .def("start_cartesian_pose_control", &Arm::startCartesianPoseControl)
        .def("start_cartesian_velocity_control", &Arm::startCartesianVelocityControl);

    bind_model<florid::Model<PantheraTraits>>(m, "PantheraModel");
    bind_model<florid::Model<WillowTraits>>(m, "WillowModel");
}
