pybind11_add_module(_pyflorid
    src/pyflorid.cpp
    src/control_types.cpp
    src/model.cpp
    src/active_control.cpp
)

target_link_libraries(_pyflorid PRIVATE florid)

target_compile_features(_pyflorid PRIVATE cxx_std_20)

# Install the compiled extension and the pure-Python package into the wheel.
install(TARGETS _pyflorid
    LIBRARY DESTINATION pyflorid
    RUNTIME DESTINATION pyflorid
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pyflorid/__init__.py
    DESTINATION pyflorid
)
