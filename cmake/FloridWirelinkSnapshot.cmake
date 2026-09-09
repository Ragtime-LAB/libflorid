# SPDX-License-Identifier: ISC
include_guard(GLOBAL)

function(lf_wirelink_snapshot_error reason)
    message(FATAL_ERROR "${reason}. Configure a development build with "
        "-DLF_ENABLE_WLC=ON and matching WLC, then build target "
        "lf_update_wirelink and commit generated/wirelink/ with the schema changes.")
endfunction()

# Fixed SDK recipe: one arm codec, shared services + host profile, name fci_arm.
# Newline normalization permits Git's Windows CRLF checkouts, including schemas
# in submodules, without requiring a generator merely to verify the snapshot.
function(lf_wirelink_snapshot_content source_root snapshot_root out_content)
    set(_inputs
        protocol/schema/wirelink/arm/fci_arm.wl
        protocol/schema/wirelink/arm/services.bind.wl
        protocol/schema/wirelink/arm/host.bind.wl)
    set(_outputs
        codec/fci_arm.c
        codec/fci_arm.h
        codec/fci_arm_values.h
        codec/fci_arm_bindings.c
        codec/fci_arm_bindings.h
        codec/fci_arm_manifest.json
        host/fci_arm_runtime.c
        host/fci_arm_runtime.h
        host/fci_arm_endpoint.h
        host/fci_arm_advanced.h
        host/fci_arm_runtime_manifest.json)
    set(_content "libflorid-wirelink-snapshot-v1\ncompiler=wlc ${WIRELINK_WLC_VERSION}\ncodegen_abi=${WIRELINK_WLC_CODEGEN_ABI}\nruntime=fci_arm\nrole=host\nhash=sha256-lf\n")
    set(_depends "")
    foreach(_kind IN ITEMS inputs outputs)
        if(_kind STREQUAL "inputs")
            set(_base "${source_root}")
        else()
            set(_base "${snapshot_root}")
        endif()
        foreach(_relative IN LISTS _${_kind})
            set(_path "${_base}/${_relative}")
            if(NOT EXISTS "${_path}")
                lf_wirelink_snapshot_error("Missing ${_path}")
            endif()
            file(READ "${_path}" _text)
            string(REPLACE "\r\n" "\n" _text "${_text}")
            string(SHA256 _digest "${_text}")
            string(APPEND _content "${_kind}/${_relative}=${_digest}\n")
            list(APPEND _depends "${_path}")
        endforeach()
    endforeach()
    foreach(_relative IN ITEMS codec/fci_arm_manifest.json host/fci_arm_runtime_manifest.json)
        file(READ "${snapshot_root}/${_relative}" _json)
        string(JSON _name GET "${_json}" compiler name)
        string(JSON _version GET "${_json}" compiler version)
        string(JSON _abi GET "${_json}" compiler codegen_abi)
        if(NOT _name STREQUAL "wlc" OR
           NOT _version STREQUAL WIRELINK_WLC_VERSION OR
           NOT _abi STREQUAL WIRELINK_WLC_CODEGEN_ABI)
            lf_wirelink_snapshot_error("${_relative} does not match the selected Wirelink compiler/ABI")
        endif()
    endforeach()
    set(${out_content} "${_content}" PARENT_SCOPE)
    set(LF_WIRELINK_SNAPSHOT_OUTPUTS "${_outputs}" PARENT_SCOPE)
    set(LF_WIRELINK_SNAPSHOT_DEPENDS "${_depends}" PARENT_SCOPE)
endfunction()
