include_guard(GLOBAL)

# One deployment layout for all arm consumers, including checked-in snapshots.
# Keep USB burst capacity and eight independently pending application calls.
function(fci_protocol_configure_arm_endpoint target runtime_name)
    string(TOUPPER "${runtime_name}" _prefix)
    target_compile_definitions("${target}" PUBLIC
        "${_prefix}_ENDPOINT_RPC_CAPACITY=8U"
        "${_prefix}_ENDPOINT_RX_FIFO_CAPACITY=4096U")
endfunction()

# The composed product accepts a 2048-byte bulk payload. Room for more than
# three complete COBS frames lets USB bursts coexist with management traffic.
function(fci_protocol_configure_device_endpoint target runtime_name)
    string(TOUPPER "${runtime_name}" _prefix)
    target_compile_definitions("${target}" PUBLIC
        "${_prefix}_ENDPOINT_RPC_CAPACITY=10U"
        "${_prefix}_ENDPOINT_RX_FIFO_CAPACITY=8192U")
endfunction()

function(_fci_protocol_prepare_wirelink)
    set(_wirelink_source_dir "${FCI_PROTOCOL_WIRELINK_SOURCE_DIR}")
    if(NOT _wirelink_source_dir AND DEFINED ZEPHYR_WIRELINK_MODULE_DIR)
        set(_wirelink_source_dir "${ZEPHYR_WIRELINK_MODULE_DIR}")
    endif()

    if(NOT TARGET Wirelink::wirelink AND NOT TARGET wirelink)
        if(_wirelink_source_dir)
            get_filename_component(_wirelink_source_dir
                "${_wirelink_source_dir}" ABSOLUTE)
            if(NOT EXISTS "${_wirelink_source_dir}/CMakeLists.txt")
                message(FATAL_ERROR
                    "FCI_PROTOCOL_WIRELINK_SOURCE_DIR is not a Wirelink tree: "
                    "${_wirelink_source_dir}")
            endif()
            add_subdirectory("${_wirelink_source_dir}"
                "${CMAKE_CURRENT_BINARY_DIR}/_deps/wirelink"
                EXCLUDE_FROM_ALL)
        else()
            find_package(Wirelink CONFIG REQUIRED)
        endif()
    endif()

    if(NOT COMMAND wirelink_wlc_generate)
        include(WirelinkWlc OPTIONAL)
    endif()
    if(NOT COMMAND wirelink_wlc_generate AND _wirelink_source_dir)
        include("${_wirelink_source_dir}/cmake/WirelinkWlc.cmake" OPTIONAL)
    endif()
    if(NOT COMMAND wirelink_wlc_generate AND Wirelink_DIR
       AND EXISTS "${Wirelink_DIR}/WirelinkWlc.cmake")
        include("${Wirelink_DIR}/WirelinkWlc.cmake")
    endif()
    if(NOT COMMAND wirelink_wlc_generate)
        message(FATAL_ERROR
            "WirelinkWlc.cmake is unavailable; use a Wirelink package/source "
            "tree that installs the WLC CMake helper")
    endif()

endfunction()

# Generate one FCI runtime for one binding role. All runtimes for a component
# share one schema codec target. Repeated calls use unique TARGET names and a
# unique RUNTIME_NAME when they will be linked into the same image; the
# canonical invocation preserves the fci_<component> C namespace.
function(fci_protocol_generate_wirelink_component)
    set(_options NO_ALIAS)
    set(_one_value_args COMPONENT ROLE TARGET ALIAS OUTPUT_DIR RUNTIME_NAME)
    cmake_parse_arguments(FCI "${_options}" "${_one_value_args}" "" ${ARGN})

    if(FCI_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "fci_protocol_generate_wirelink_component received unknown "
            "arguments: ${FCI_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT FCI_COMPONENT)
        message(FATAL_ERROR
            "fci_protocol_generate_wirelink_component requires COMPONENT")
    endif()
    if(NOT FCI_ROLE)
        message(FATAL_ERROR
            "fci_protocol_generate_wirelink_component requires ROLE")
    endif()

    set(_supported_components arm upgrade dual device)
    list(FIND _supported_components "${FCI_COMPONENT}" _component_index)
    if(_component_index EQUAL -1)
        message(FATAL_ERROR
            "FCI component must be arm, upgrade, dual, or device; got "
            "'${FCI_COMPONENT}'")
    endif()
    if(NOT FCI_ROLE STREQUAL "firmware" AND NOT FCI_ROLE STREQUAL "host")
        message(FATAL_ERROR
            "FCI role must be firmware or host; got '${FCI_ROLE}'")
    endif()
    if(FCI_NO_ALIAS AND FCI_ALIAS)
        message(FATAL_ERROR "NO_ALIAS and ALIAS are mutually exclusive")
    endif()

    if(FCI_TARGET)
        set(_target "${FCI_TARGET}")
        set(_custom_target TRUE)
    else()
        set(_target "fci_protocol_wirelink_${FCI_COMPONENT}")
        set(_custom_target FALSE)
    endif()

    if(FCI_NO_ALIAS)
        set(_alias "")
    elseif(FCI_ALIAS)
        set(_alias "${FCI_ALIAS}")
    elseif(NOT _custom_target)
        set(_alias "fci_protocol::${FCI_COMPONENT}")
    else()
        set(_alias "")
    endif()

    set(_fci_protocol_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    set(_schema
        "${_fci_protocol_root}/schema/wirelink/${FCI_COMPONENT}/fci_${FCI_COMPONENT}.wl")
    set(_profiles
        "${_fci_protocol_root}/schema/wirelink/${FCI_COMPONENT}/${FCI_ROLE}.bind.wl")
    if(FCI_COMPONENT STREQUAL "arm")
        list(PREPEND _profiles
            "${_fci_protocol_root}/schema/wirelink/arm/services.bind.wl")
    elseif(FCI_COMPONENT STREQUAL "device")
        list(PREPEND _profiles
            "${_fci_protocol_root}/schema/wirelink/arm/services.bind.wl"
            "${_fci_protocol_root}/schema/wirelink/arm/${FCI_ROLE}.bind.wl"
            "${_fci_protocol_root}/schema/wirelink/upgrade/${FCI_ROLE}.bind.wl"
            "${_fci_protocol_root}/schema/wirelink/underwater/services.bind.wl"
            "${_fci_protocol_root}/schema/wirelink/single/services.bind.wl")
    endif()
    set(_codec_target "fci_protocol_wirelink_${FCI_COMPONENT}_codec")
    if(FCI_OUTPUT_DIR)
        get_filename_component(_output_dir "${FCI_OUTPUT_DIR}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    elseif(FCI_PROTOCOL_GENERATED_BASE_DIR)
        set(_output_dir
            "${FCI_PROTOCOL_GENERATED_BASE_DIR}/${FCI_COMPONENT}/${FCI_ROLE}/${_target}")
    else()
        set(_output_dir
            "${CMAKE_CURRENT_BINARY_DIR}/generated/wirelink/${FCI_COMPONENT}/${FCI_ROLE}/${_target}")
    endif()
    if(FCI_PROTOCOL_GENERATED_BASE_DIR)
        set(_codec_output_dir
            "${FCI_PROTOCOL_GENERATED_BASE_DIR}/${FCI_COMPONENT}/codec/${_codec_target}")
    else()
        set(_codec_output_dir
            "${CMAKE_CURRENT_BINARY_DIR}/generated/wirelink/${FCI_COMPONENT}/codec/${_codec_target}")
    endif()
    if(FCI_RUNTIME_NAME)
        set(_runtime_name "${FCI_RUNTIME_NAME}")
    else()
        set(_runtime_name "fci_${FCI_COMPONENT}")
    endif()

    if(TARGET "${_target}")
        get_target_property(_existing_component "${_target}"
            FCI_PROTOCOL_COMPONENT)
        get_target_property(_existing_role "${_target}" FCI_PROTOCOL_ROLE)
        get_target_property(_existing_runtime_name "${_target}"
            FCI_PROTOCOL_RUNTIME_NAME)
        if(NOT "${_existing_component}" STREQUAL "${FCI_COMPONENT}" OR
           NOT "${_existing_role}" STREQUAL "${FCI_ROLE}" OR
           NOT "${_existing_runtime_name}" STREQUAL "${_runtime_name}")
            message(FATAL_ERROR
                "Target '${_target}' already exists with a different FCI "
                "component, role, or runtime name")
        endif()
    else()
        _fci_protocol_prepare_wirelink()
        if(NOT TARGET "${_codec_target}")
            wirelink_wlc_generate_codec(
                TARGET "${_codec_target}"
                SCHEMA "${_schema}"
                OUTPUT_DIR "${_codec_output_dir}"
                WLC_EXECUTABLE "${WLC_EXECUTABLE}")
            set_property(TARGET "${_codec_target}" PROPERTY
                FCI_PROTOCOL_COMPONENT "${FCI_COMPONENT}")
        endif()
        wirelink_wlc_generate_runtime(
            TARGET "${_target}"
            CODEC_TARGET "${_codec_target}"
            PROFILES ${_profiles}
            OUTPUT_DIR "${_output_dir}"
            RUNTIME_NAME "${_runtime_name}")
        if(FCI_COMPONENT STREQUAL "arm")
            fci_protocol_configure_arm_endpoint("${_target}" "${_runtime_name}")
        elseif(FCI_COMPONENT STREQUAL "device")
            fci_protocol_configure_device_endpoint("${_target}" "${_runtime_name}")
        endif()
        set_property(TARGET "${_target}" PROPERTY
            FCI_PROTOCOL_COMPONENT "${FCI_COMPONENT}")
        set_property(TARGET "${_target}" PROPERTY
            FCI_PROTOCOL_ROLE "${FCI_ROLE}")
        set_property(TARGET "${_target}" PROPERTY
            FCI_PROTOCOL_CODEC_TARGET "${_codec_target}")
        set_property(TARGET "${_target}" PROPERTY
            FCI_PROTOCOL_RUNTIME_NAME "${_runtime_name}")
    endif()

    if(_alias)
        if(TARGET "${_alias}")
            get_target_property(_aliased_target "${_alias}" ALIASED_TARGET)
            if(NOT "${_aliased_target}" STREQUAL "${_target}")
                message(FATAL_ERROR
                    "Alias '${_alias}' already names '${_aliased_target}', "
                    "not '${_target}'")
            endif()
        else()
            add_library("${_alias}" ALIAS "${_target}")
        endif()
    endif()

endfunction()

# Generate the configured component set. Consumers link the explicit
# fci_protocol::<component> target that owns their generated contract.
function(fci_protocol_enable_wirelink)
    get_property(_has_deferred_components GLOBAL PROPERTY
        FCI_PROTOCOL_DEFERRED_COMPONENTS SET)
    if(_has_deferred_components)
        get_property(_components GLOBAL PROPERTY
            FCI_PROTOCOL_DEFERRED_COMPONENTS)
        get_property(_role GLOBAL PROPERTY FCI_PROTOCOL_DEFERRED_ROLE)
    else()
        set(_components ${FCI_PROTOCOL_WIRELINK_COMPONENTS})
        set(_role "${FCI_PROTOCOL_WIRELINK_ROLE}")
    endif()
    if(NOT _components)
        return()
    endif()

    foreach(_component IN LISTS _components)
        fci_protocol_generate_wirelink_component(
            COMPONENT "${_component}"
            ROLE "${_role}")
    endforeach()

endfunction()
