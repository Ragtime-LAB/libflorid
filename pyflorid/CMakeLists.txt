pybind11_add_module(_pyflorid
    src/pyflorid.cpp
    src/underwater.cpp
    src/single.cpp
)

target_link_libraries(_pyflorid PRIVATE florid florid_wirelink_host)

target_compile_features(_pyflorid PRIVATE cxx_std_20)

# Install the compiled extension and the pure-Python package into the wheel.
install(TARGETS _pyflorid
    LIBRARY DESTINATION pyflorid
    RUNTIME DESTINATION pyflorid
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pyflorid/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/pyflorid/single.py
    ${CMAKE_CURRENT_SOURCE_DIR}/pyflorid/_single_serial.py
    ${CMAKE_CURRENT_SOURCE_DIR}/pyflorid/_single_codec.py
    DESTINATION pyflorid
)

if(BUILD_TESTS)
    add_test(NAME underwater_single_python
        COMMAND "${Python_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/test_single_python.py"
            "$<TARGET_FILE_DIR:_pyflorid>" "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME underwater_python
        COMMAND "${Python_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/test_underwater_python.py"
            "$<TARGET_FILE_DIR:_pyflorid>" "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME underwater_console_controller
        COMMAND "${Python_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/underwater_console/tests/test_controller.py")
endif()
