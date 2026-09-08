cmake_minimum_required(VERSION 3.21)

project(florid VERSION 0.3.0 LANGUAGES CXX C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS   "Build unit tests" OFF)
option(BUILD_EXAMPLES "Build SDK examples" ON)
option(BUILD_PYFLORID "Build Python bindings via pybind11" OFF)
option(BUILD_MPC      "Enable MPC via acados" OFF)
option(LF_ENABLE_WLC "Generate FCI bindings with WLC instead of the checked-in snapshot" OFF)
set(WIRELINK_SOURCE_DIR "" CACHE PATH
    "Optional Wirelink source-tree override used by libflorid")
set(WLC_EXECUTABLE "" CACHE FILEPATH
    "Host wlc executable used to generate the FCI Wirelink bindings")

# USB is the product transport. Build Astrial's libusb backend before adding
# Wirelink so its zero-copy adapter can bind to the existing Astrial targets.
set(ASTRIAL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ASTRIAL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASTRIAL_BUILD_USB ON CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/astrial)
set(WIRELINK_BUILD_ASTRIAL_USB_ADAPTER ON CACHE BOOL "" FORCE)
set(WIRELINK_BUILD_ASTRIAL_ADAPTER ON CACHE BOOL "" FORCE)
set(WIRELINK_BUILD_ASIO_UDP_ADAPTER ON CACHE BOOL "" FORCE)
set(WIRELINK_BUILD_HOST_RUNTIME ON CACHE BOOL "" FORCE)
set(WIRELINK_BUILD_PLATFORM ON CACHE BOOL "" FORCE)
set(WIRELINK_ASIO_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/astrial/3rdparty/asio/include"
    CACHE PATH "" FORCE)

if(BUILD_MPC)
    set(ACADOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/acados" CACHE PATH "acados source directory")
    set(ACADOS_INSTALL_DIR "${ACADOS_SOURCE_DIR}" CACHE PATH "acados install directory")
    add_subdirectory(3rdparty/acados ${CMAKE_BINARY_DIR}/acados
        EXCLUDE_FROM_ALL)
endif()

set(FCI_PROTOCOL_WIRELINK_ROLE host CACHE STRING
    "Generate the host-side FCI Wirelink runtime" FORCE)
set(FCI_PROTOCOL_GENERATED_BASE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated/wirelink" CACHE PATH
    "Output root for FCI Wirelink artifacts when LF_ENABLE_WLC is ON")
if(NOT WIRELINK_SOURCE_DIR STREQUAL "")
    set(FCI_PROTOCOL_WIRELINK_SOURCE_DIR "${WIRELINK_SOURCE_DIR}" CACHE PATH
        "Wirelink source tree used by fci_protocol" FORCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wirelink/CMakeLists.txt")
    set(FCI_PROTOCOL_WIRELINK_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wirelink" CACHE PATH
        "Wirelink source tree used by fci_protocol" FORCE)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FloridWirelink.cmake")
lf_prepare_wirelink()
# Asio is header-only but its inline types are shared across the final binary.
# Astrial may select io_uring via asio_standalone; compile the UDP adapter with
# that exact target too. Mixing epoll/io_uring definitions causes ODR violations
# (notably socket-close hangs in unoptimized/sanitized builds).
target_link_libraries(wirelink_asio_udp PRIVATE asio_standalone)
find_package(Threads REQUIRED)

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
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
    find_package(pybind11 REQUIRED)
    add_subdirectory(pyflorid)
endif()
