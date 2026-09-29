if(NOT TARGET fci_protocol::arm)
    fci_protocol_generate_wirelink_component(
        COMPONENT arm
        ROLE "${FCI_PROTOCOL_WIRELINK_ROLE}")
endif()

fci_protocol_generate_wirelink_component(
    COMPONENT arm
    ROLE host
    TARGET fci_protocol_wirelink_arm_host_smoke
    RUNTIME_NAME fci_arm_host
    NO_ALIAS)
fci_protocol_generate_wirelink_component(
    COMPONENT arm
    ROLE firmware
    TARGET fci_protocol_wirelink_arm_firmware_smoke
    RUNTIME_NAME fci_arm_firmware
    NO_ALIAS)
add_executable(fci_protocol_wirelink_asymmetric_roles_smoke
    wirelink_asymmetric_roles_smoke.c)
target_link_libraries(fci_protocol_wirelink_asymmetric_roles_smoke PRIVATE
    fci_protocol_wirelink_arm_host_smoke
    fci_protocol_wirelink_arm_firmware_smoke)
add_test(NAME fci_protocol_wirelink_asymmetric_roles_smoke
    COMMAND fci_protocol_wirelink_asymmetric_roles_smoke)
add_executable(fci_protocol_managed_endpoints wirelink_managed_endpoints.c)
target_link_libraries(fci_protocol_managed_endpoints PRIVATE
    fci_protocol_wirelink_arm_host_smoke fci_protocol_wirelink_arm_firmware_smoke)
add_test(NAME fci_protocol_managed_endpoints COMMAND fci_protocol_managed_endpoints)

foreach(_role IN ITEMS host firmware)
    fci_protocol_generate_wirelink_component(
        COMPONENT device ROLE ${_role}
        TARGET fci_protocol_wirelink_device_${_role}_smoke
        RUNTIME_NAME fci_device_${_role} NO_ALIAS)
endforeach()
add_executable(fci_protocol_underwater_endpoints wirelink_underwater_endpoints.c)
target_link_libraries(fci_protocol_underwater_endpoints PRIVATE
    fci_protocol_wirelink_device_host_smoke
    fci_protocol_wirelink_device_firmware_smoke)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fci_protocol_underwater_endpoints PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME fci_protocol_underwater_endpoints COMMAND fci_protocol_underwater_endpoints)

foreach(_configured_component IN LISTS FCI_PROTOCOL_WIRELINK_COMPONENTS)
    if(NOT TARGET fci_protocol_wirelink_${_configured_component} OR
       NOT TARGET fci_protocol::${_configured_component})
        message(FATAL_ERROR
            "missing target/alias for ${_configured_component} component")
    endif()
endforeach()
# Exercise the helper's safe fallback when it is included without the
# top-level generated-base cache variable.
set(_saved_generated_base_dir "${FCI_PROTOCOL_GENERATED_BASE_DIR}")
set(FCI_PROTOCOL_GENERATED_BASE_DIR "")
fci_protocol_generate_wirelink_component(
    COMPONENT upgrade
    ROLE host
    TARGET fci_protocol_wirelink_default_path_smoke
    NO_ALIAS)
fci_protocol_generate_wirelink_component(
    COMPONENT upgrade
    ROLE host
    TARGET fci_protocol_wirelink_default_path_second_smoke
    NO_ALIAS)
get_target_property(_default_generated_dir
    fci_protocol_wirelink_default_path_smoke WIRELINK_WLC_GENERATED_DIR)
get_target_property(_second_default_generated_dir
    fci_protocol_wirelink_default_path_second_smoke
    WIRELINK_WLC_GENERATED_DIR)
set(_default_generated_root
    "${CMAKE_CURRENT_BINARY_DIR}/generated/wirelink/upgrade/host")
set(_expected_default_generated_dir
    "${_default_generated_root}/fci_protocol_wirelink_default_path_smoke")
set(_expected_second_default_generated_dir
    "${_default_generated_root}/fci_protocol_wirelink_default_path_second_smoke")
if(NOT "${_default_generated_dir}" STREQUAL
   "${_expected_default_generated_dir}")
    message(FATAL_ERROR
        "component helper defaulted to '${_default_generated_dir}', expected "
        "'${_expected_default_generated_dir}'")
endif()
if(NOT "${_second_default_generated_dir}" STREQUAL
   "${_expected_second_default_generated_dir}")
    message(FATAL_ERROR
        "second component helper defaulted to "
        "'${_second_default_generated_dir}', expected "
        "'${_expected_second_default_generated_dir}'")
endif()
if("${_default_generated_dir}" STREQUAL
   "${_second_default_generated_dir}")
    message(FATAL_ERROR
        "unique component targets share '${_default_generated_dir}'")
endif()

set(_explicit_generated_dir
    "${CMAKE_CURRENT_BINARY_DIR}/explicit/wirelink/upgrade-host")
fci_protocol_generate_wirelink_component(
    COMPONENT upgrade
    ROLE host
    TARGET fci_protocol_wirelink_explicit_path_smoke
    OUTPUT_DIR "${_explicit_generated_dir}"
    NO_ALIAS)
get_target_property(_actual_explicit_generated_dir
    fci_protocol_wirelink_explicit_path_smoke WIRELINK_WLC_GENERATED_DIR)
if(NOT "${_actual_explicit_generated_dir}" STREQUAL
   "${_explicit_generated_dir}")
    message(FATAL_ERROR
        "explicit component output changed to "
        "'${_actual_explicit_generated_dir}', expected "
        "'${_explicit_generated_dir}'")
endif()
set(FCI_PROTOCOL_GENERATED_BASE_DIR "${_saved_generated_base_dir}")
unset(_saved_generated_base_dir)

add_executable(fci_protocol_wirelink_schema_smoke
    wirelink_schema_smoke.c)
target_link_libraries(fci_protocol_wirelink_schema_smoke
    PRIVATE fci_protocol::arm)
add_test(NAME fci_protocol_wirelink_schema_smoke
    COMMAND fci_protocol_wirelink_schema_smoke)

add_executable(fci_protocol_wirelink_required_smoke
    wirelink_required_smoke.c)
target_link_libraries(fci_protocol_wirelink_required_smoke
    PRIVATE fci_protocol::arm)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fci_protocol_wirelink_schema_smoke
        PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(fci_protocol_wirelink_required_smoke
        PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME fci_protocol_wirelink_required_smoke
    COMMAND fci_protocol_wirelink_required_smoke)

if(TARGET Wirelink::wirelink)
    set(_wirelink_core_target Wirelink::wirelink)
elseif(TARGET wirelink)
    set(_wirelink_core_target wirelink)
else()
    message(FATAL_ERROR "FCI schema tests require the Wirelink core target")
endif()

get_target_property(_fci_protocol_test_wlc
    fci_protocol_wirelink_arm WIRELINK_WLC_EXECUTABLE)
if(NOT _fci_protocol_test_wlc OR
   _fci_protocol_test_wlc STREQUAL "_fci_protocol_test_wlc-NOTFOUND")
    message(FATAL_ERROR
        "generated FCI target did not retain its resolved WLC executable")
endif()

foreach(_component IN ITEMS arm upgrade dual device)
    foreach(_role IN ITEMS firmware host)
        set(_module "fci_${_component}")
        set(_profile
            "${CMAKE_CURRENT_SOURCE_DIR}/../schema/wirelink/${_component}/${_role}.bind.wl")
        set(_services "")
        if(_component STREQUAL "arm")
            set(_services
                "${CMAKE_CURRENT_SOURCE_DIR}/../schema/wirelink/arm/services.bind.wl")
        elseif(_component STREQUAL "device")
            set(_services
                "${CMAKE_CURRENT_SOURCE_DIR}/../schema/wirelink/arm/services.bind.wl"
                "${CMAKE_CURRENT_SOURCE_DIR}/../schema/wirelink/arm/${_role}.bind.wl"
                "${CMAKE_CURRENT_SOURCE_DIR}/../schema/wirelink/upgrade/${_role}.bind.wl"
                "${CMAKE_CURRENT_SOURCE_DIR}/../schema/wirelink/underwater/services.bind.wl")
        endif()
        set(_schema
            "${CMAKE_CURRENT_SOURCE_DIR}/../schema/wirelink/${_component}/${_module}.wl")
        set(_output_dir
            "${CMAKE_CURRENT_BINARY_DIR}/generated/${_component}/${_role}")
        add_test(
            NAME fci_protocol_wlc_compile_${_component}_${_role}
            COMMAND "${CMAKE_COMMAND}"
                "-DWLC_EXECUTABLE=${_fci_protocol_test_wlc}"
                "-DSCHEMA=${_schema}"
                "-DPROFILE=${_profile}"
                "-DSERVICES_PROFILE=${_services}"
                "-DMODULE=${_module}"
                "-DOUTPUT_DIR=${_output_dir}"
                "-DC_COMPILER=${CMAKE_C_COMPILER}"
                "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
                "-DC_COMPILER_ID=${CMAKE_C_COMPILER_ID}"
                "-DCXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}"
                "-DWIRELINK_INCLUDE_DIRS=$<TARGET_PROPERTY:${_wirelink_core_target},INTERFACE_INCLUDE_DIRECTORIES>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/CompileWirelinkSchema.cmake")
    endforeach()
endforeach()
