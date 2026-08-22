pybind11_add_module(_pyflorid
    src/pyflorid.cpp
    src/control_types.cpp
    src/model.cpp
    src/active_control.cpp
)

target_link_libraries(_pyflorid PRIVATE florid)

target_compile_features(_pyflorid PRIVATE cxx_std_20)
