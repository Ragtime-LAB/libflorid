cmake_minimum_required(VERSION 3.20)

project(florid VERSION 0.1.0 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS   "Build unit tests" OFF)
option(BUILD_EXAMPLES "Build SDK examples" ON)
option(BUILD_PYFLORID "Build Python bindings via pybind11" OFF)
option(BUILD_MPC      "Enable MPC via acados" OFF)

include(SDKDependencies)

if(BUILD_MPC)
    set(ACADOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/acados" CACHE PATH "acados source directory")
    set(ACADOS_INSTALL_DIR "${ACADOS_SOURCE_DIR}" CACHE PATH "acados install directory")
    add_subdirectory(3rdparty/acados ${CMAKE_BINARY_DIR}/acados
        EXCLUDE_FROM_ALL)
    add_compile_definitions(FLORID_HAS_MPC)
endif()

add_subdirectory(protocol)
add_subdirectory(3rdparty/astrial)
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_PYFLORID)
    find_package(pybind11 REQUIRED)
    find_package(Python COMPONENTS Interpreter Development NumPy REQUIRED)
    add_subdirectory(pyflorid)
endif()
