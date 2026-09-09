# SPDX-License-Identifier: ISC
cmake_minimum_required(VERSION 3.21)

# Exercise the real default CMake path without needing USB dependencies or WLC.
# Only Wirelink's library target is stubbed; its compiler resolver remains real,
# with an invalid explicit executable so any accidental invocation must fail.
set(_source "${LF_TEST_DIR}/source")
file(MAKE_DIRECTORY "${_source}/protocol/schema/wirelink"
    "${_source}/protocol/cmake" "${_source}/generated")
file(COPY "${LF_SOURCE_DIR}/cmake" DESTINATION "${_source}")
file(COPY "${LF_SOURCE_DIR}/protocol/cmake/FciProtocolWirelink.cmake"
    DESTINATION "${_source}/protocol/cmake")
file(COPY "${LF_SOURCE_DIR}/protocol/schema/wirelink/arm"
    "${LF_SOURCE_DIR}/protocol/schema/wirelink/upgrade"
    "${LF_SOURCE_DIR}/protocol/schema/wirelink/device"
    DESTINATION "${_source}/protocol/schema/wirelink")
file(COPY "${LF_SOURCE_DIR}/generated/wirelink" DESTINATION "${_source}/generated")
set(_project [=[
cmake_minimum_required(VERSION 3.21)
project(SnapshotConsumer LANGUAGES C)
set(LF_ENABLE_WLC OFF)
set(WLC_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/wlc-must-not-run")
set(WIRELINK_WLC_EXECUTABLE "${WLC_EXECUTABLE}")
set(WIRELINK_WLC_AUTO_DOWNLOAD OFF CACHE BOOL "" FORCE)
include("@LF_SOURCE_DIR@/3rdparty/wirelink/cmake/WirelinkWlc.cmake")
add_library(wirelink INTERFACE)
add_library(Wirelink::wirelink ALIAS wirelink)
include(cmake/FloridWirelink.cmake)
lf_prepare_wirelink()
]=])
string(CONFIGURE "${_project}" _project @ONLY)
file(WRITE "${_source}/CMakeLists.txt" "${_project}")

function(_configure_case name expected)
    execute_process(COMMAND "${CMAKE_COMMAND}" -S "${_source}"
        -B "${LF_TEST_DIR}/build" "-DCMAKE_C_COMPILER=${LF_C_COMPILER}"
        RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
    if(expected STREQUAL "PASS")
        if(NOT _result EQUAL 0)
            message(FATAL_ERROR "${name} failed: ${_out}\n${_err}")
        endif()
    elseif(_result EQUAL 0 OR NOT "${_out}\n${_err}" MATCHES "${expected}")
        message(FATAL_ERROR "${name} did not reject the snapshot: ${_out}\n${_err}")
    endif()
    message(STATUS "${name}: PASS")
endfunction()

_configure_case("valid snapshot without WLC" PASS)

foreach(_input IN ITEMS upgrade/fci_upgrade.wl device/fci_device.wl device/host.bind.wl)
    set(_path "${_source}/protocol/schema/wirelink/${_input}")
    file(READ "${_path}" _saved)
    file(APPEND "${_path}" "\n// transitive snapshot dependency probe\n")
    _configure_case("changed ${_input}" "stale or modified")
    file(WRITE "${_path}" "${_saved}")
endforeach()
set(_schema "${_source}/protocol/schema/wirelink/arm/fci_arm.wl")
file(READ "${_schema}" _original_schema)
file(APPEND "${_schema}" "\n// changed input\n")
_configure_case("changed schema" "stale or modified")
file(WRITE "${_schema}" "${_original_schema}")

set(_profile "${_source}/protocol/schema/wirelink/arm/host.bind.wl")
file(READ "${_profile}" _original_profile)
file(APPEND "${_profile}" "\n// changed profile\n")
_configure_case("changed profile" "stale or modified")
file(WRITE "${_profile}" "${_original_profile}")

set(_services "${_source}/protocol/schema/wirelink/arm/services.bind.wl")
file(READ "${_services}" _original_services)
file(APPEND "${_services}" "\n// changed shared service\n")
_configure_case("changed shared services" "stale or modified")
file(WRITE "${_services}" "${_original_services}")

set(_output "${_source}/generated/wirelink/codec/fci_device.c")
file(READ "${_output}" _original_output)
file(APPEND "${_output}" "\n/* changed generated file */\n")
_configure_case("changed generated file" "stale or modified")
file(WRITE "${_output}" "${_original_output}")
file(RENAME "${_output}" "${_output}.saved")
_configure_case("missing generated file" "Missing")
file(RENAME "${_output}.saved" "${_output}")

set(_manifest "${_source}/generated/wirelink/codec/fci_device_manifest.json")
file(READ "${_manifest}" _original_manifest)
string(JSON _changed SET "${_original_manifest}" compiler codegen_abi 999)
file(WRITE "${_manifest}" "${_changed}")
_configure_case("wrong compiler ABI" "does not match")
file(WRITE "${_manifest}" "${_original_manifest}")

# Git can independently convert line endings in the protocol submodule.
string(REPLACE "\r\n" "\n" _lf "${_original_schema}")
string(REPLACE "\n" "\r\n" _crlf "${_lf}")
file(WRITE "${_schema}" "${_crlf}")
_configure_case("CRLF schema checkout" PASS)
string(REPLACE "\r\n" "\n" _lf "${_original_services}")
string(REPLACE "\n" "\r\n" _crlf "${_lf}")
file(WRITE "${_services}" "${_crlf}")
_configure_case("CRLF shared services checkout" PASS)
