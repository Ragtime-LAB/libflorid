#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "florid/Model.hpp"
#include "florid/traits/PantheraTraits.hpp"

namespace py = pybind11;

using PantheraModel = florid::Model<florid::PantheraTraits>;

// Helper: wrap float* output method → return numpy array
template <int N>
py::array_t<float> s_call_model(void (PantheraModel::*s_fn)(const float*, float*) const,
                                 PantheraModel& s_model,
                                 py::array_t<float, py::array::c_style | py::array::forcecast> s_q) {
    auto s_out = py::array_t<float>(N);
    (s_model.*s_fn)(s_q.data(), s_out.mutable_data());
    return s_out;
}

void bind_model(py::module_& m) {
    py::class_<PantheraModel>(m, "Model")
        .def(py::init<>())
        .def("fk", [](PantheraModel& s_m, py::array_t<float, py::array::c_style | py::array::forcecast> s_q) {
            return s_call_model<16>(&PantheraModel::forwardKinematics, s_m, s_q);
        }, py::arg("q"))
        .def("zero_jacobian", [](PantheraModel& s_m, py::array_t<float, py::array::c_style | py::array::forcecast> s_q) {
            return s_call_model<36>(&PantheraModel::zeroJacobian, s_m, s_q);
        }, py::arg("q"))
        .def("body_jacobian", [](PantheraModel& s_m, py::array_t<float, py::array::c_style | py::array::forcecast> s_q) {
            return s_call_model<36>(&PantheraModel::bodyJacobian, s_m, s_q);
        }, py::arg("q"))
        .def("mass", [](PantheraModel& s_m, py::array_t<float, py::array::c_style | py::array::forcecast> s_q) {
            return s_call_model<36>(&PantheraModel::mass, s_m, s_q);
        }, py::arg("q"))
        .def("gravity", [](PantheraModel& s_m,
                           py::array_t<float, py::array::c_style | py::array::forcecast> s_q,
                           py::array_t<float, py::array::c_style | py::array::forcecast> s_gvec) {
            auto s_out = py::array_t<float>(6);
            s_m.gravity(s_q.data(), s_gvec.data(), s_out.mutable_data());
            return s_out;
        }, py::arg("q"), py::arg("g_vec"))
        .def("coriolis", [](PantheraModel& s_m,
                            py::array_t<float, py::array::c_style | py::array::forcecast> s_q,
                            py::array_t<float, py::array::c_style | py::array::forcecast> s_dq) {
            auto s_out = py::array_t<float>(6);
            s_m.coriolis(s_q.data(), s_dq.data(), s_out.mutable_data());
            return s_out;
        }, py::arg("q"), py::arg("dq"));
}
