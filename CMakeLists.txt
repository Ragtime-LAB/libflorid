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
option(FLORID_BUILD_WIRELINK_HOST
    "Build the experimental Wirelink single-owner host executor" OFF)
set(WIRELINK_SOURCE_DIR "" CACHE PATH
    "Optional Wirelink source tree used by the host executor")

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

if(NOT WIRELINK_SOURCE_DIR STREQUAL "")
    set(FLORID_BUILD_WIRELINK_HOST ON)
endif()

if(FLORID_BUILD_WIRELINK_HOST)
    if(NOT TARGET Wirelink::wirelink)
        if(NOT WIRELINK_SOURCE_DIR STREQUAL "")
            if(NOT EXISTS "${WIRELINK_SOURCE_DIR}/CMakeLists.txt")
                message(FATAL_ERROR
                    "WIRELINK_SOURCE_DIR does not contain a CMake project: "
                    "${WIRELINK_SOURCE_DIR}")
            endif()
            if(CMAKE_VERSION VERSION_LESS 3.21)
                message(FATAL_ERROR
                    "Building Wirelink from source requires CMake 3.21 or newer")
            endif()
            set(WIRELINK_BUILD_EXAMPLES OFF)
            set(WIRELINK_INSTALL OFF)
            add_subdirectory("${WIRELINK_SOURCE_DIR}"
                "${CMAKE_BINARY_DIR}/_deps/wirelink" EXCLUDE_FROM_ALL)
        else()
            find_package(Wirelink CONFIG REQUIRED)
        endif()
    endif()
    find_package(Threads REQUIRED)
endif()

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
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
    add_subdirectory(pyflorid)
endif()
