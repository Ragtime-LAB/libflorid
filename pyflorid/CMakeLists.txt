pybind11_add_module(_pyflorid
    src/pyflorid.cpp
    src/underwater.cpp
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

if(BUILD_TESTS)
    add_test(NAME underwater_python
        COMMAND "${Python_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/test_underwater_python.py"
            "$<TARGET_FILE_DIR:_pyflorid>" "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME underwater_console_controller
        COMMAND "${Python_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/underwater_console/tests/test_controller.py")
endif()
