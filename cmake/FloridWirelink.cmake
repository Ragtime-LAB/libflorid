# SPDX-License-Identifier: ISC
include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/../protocol/cmake/FciProtocolWirelink.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/FloridWirelinkSnapshot.cmake")

function(lf_prepare_wirelink)
    set(_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    get_filename_component(_root "${_root}" ABSOLUTE)
    set(_snapshot "${_root}/generated/wirelink")
    _fci_protocol_prepare_wirelink()

    if(LF_ENABLE_WLC)
        fci_protocol_generate_wirelink_component(COMPONENT arm ROLE host)
        get_target_property(_codec_dir fci_protocol_wirelink_arm_codec
            WIRELINK_WLC_GENERATED_DIR)
        get_target_property(_runtime_dir fci_protocol_wirelink_arm
            WIRELINK_WLC_GENERATED_DIR)
        foreach(_mode IN ITEMS update check)
            add_custom_target(lf_${_mode}_wirelink
                COMMAND "${CMAKE_COMMAND}"
                    "-DLF_SNAPSHOT_MODE=${_mode}"
                    "-DLF_SOURCE_DIR=${_root}"
                    "-DLF_CODEC_DIR=${_codec_dir}"
                    "-DLF_RUNTIME_DIR=${_runtime_dir}"
                    "-DLF_STAGING_DIR=${CMAKE_CURRENT_BINARY_DIR}/wirelink-snapshot"
                    "-DWIRELINK_WLC_VERSION=${WIRELINK_WLC_VERSION}"
                    "-DWIRELINK_WLC_CODEGEN_ABI=${WIRELINK_WLC_CODEGEN_ABI}"
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/SyncWirelinkSnapshot.cmake"
                DEPENDS fci_protocol_wirelink_arm_wlc_codegen
                COMMENT "${_mode} the checked-in FCI Wirelink snapshot"
                VERBATIM)
        endforeach()
        message(STATUS "libflorid: generating FCI bindings with WLC")
        return()
    endif()

    # This path only reads files. It must never resolve or execute a host WLC.
    lf_wirelink_snapshot_content("${_root}" "${_snapshot}" _expected)
    set(_manifest "${_snapshot}/snapshot.txt")
    if(NOT EXISTS "${_manifest}")
        lf_wirelink_snapshot_error("Missing ${_manifest}")
    endif()
    file(READ "${_manifest}" _actual)
    string(REPLACE "\r\n" "\n" _actual "${_actual}")
    if(NOT _actual STREQUAL _expected)
        lf_wirelink_snapshot_error("FCI Wirelink snapshot is stale or modified")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${_manifest}" ${LF_WIRELINK_SNAPSHOT_DEPENDS})

    add_library(fci_protocol_wirelink_arm_codec STATIC
        "${_snapshot}/codec/fci_arm.c"
        "${_snapshot}/codec/fci_arm_bindings.c")
    target_include_directories(fci_protocol_wirelink_arm_codec PUBLIC
        "${_snapshot}/codec")
    target_compile_features(fci_protocol_wirelink_arm_codec PUBLIC c_std_11)
    target_link_libraries(fci_protocol_wirelink_arm_codec PUBLIC Wirelink::wirelink)
    add_library(fci_protocol_wirelink_arm STATIC
        "${_snapshot}/host/fci_arm_runtime.c")
    target_include_directories(fci_protocol_wirelink_arm PUBLIC "${_snapshot}/host")
    target_compile_features(fci_protocol_wirelink_arm PUBLIC c_std_11)
    target_link_libraries(fci_protocol_wirelink_arm PUBLIC fci_protocol_wirelink_arm_codec)
    add_library(fci_protocol::arm ALIAS fci_protocol_wirelink_arm)
    message(STATUS "libflorid: using checked-in FCI bindings (WLC disabled)")
endfunction()
