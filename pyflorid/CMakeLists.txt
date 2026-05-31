cmake_minimum_required(VERSION 3.20)

find_package(pybind11 REQUIRED)

pybind11_add_module(_pyflorid
    src/pyflorid.cpp
)

target_include_directories(_pyflorid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty/BasicLinearAlgebra
)

target_link_libraries(_pyflorid PRIVATE florid)
