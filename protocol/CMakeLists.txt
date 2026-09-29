cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(fci_protocol LANGUAGES C CXX)
endif()

option(FCI_PROTOCOL_BUILD_SCHEMA_TESTS
    "Build and register FCI Wirelink schema smoke tests" OFF)
set(FCI_PROTOCOL_WIRELINK_ROLE "firmware" CACHE STRING
    "Generated FCI Wirelink binding role: firmware or host")
set_property(CACHE FCI_PROTOCOL_WIRELINK_ROLE PROPERTY STRINGS firmware host)
set(FCI_PROTOCOL_WIRELINK_COMPONENTS "arm" CACHE STRING
    "FCI Wirelink components to generate: arm, upgrade, dual, and/or device")
set(FCI_PROTOCOL_WIRELINK_SOURCE_DIR "" CACHE PATH
    "Optional Wirelink source tree used when no Wirelink target/package exists")
set(FCI_PROTOCOL_GENERATED_BASE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated/wirelink" CACHE PATH
    "Base directory for generated FCI Wirelink artifacts")
set(WLC_EXECUTABLE "" CACHE FILEPATH
    "Host wlc executable used to generate Wirelink C sources")

if(ZEPHYR_BASE)
    set(FCI_PROTOCOL_WIRELINK_ROLE firmware)
    set(FCI_PROTOCOL_WIRELINK_COMPONENTS "")
    foreach(_component IN ITEMS ARM UPGRADE DUAL DEVICE)
        if(CONFIG_FCI_PROTOCOL_${_component})
            string(TOLOWER "${_component}" _component_name)
            list(APPEND FCI_PROTOCOL_WIRELINK_COMPONENTS "${_component_name}")
        endif()
    endforeach()
    # Deferred calls execute in the application directory scope. Global
    # properties preserve the Kconfig selection without mutating desktop
    # cache defaults or depending on directory-local variables.
    set_property(GLOBAL PROPERTY FCI_PROTOCOL_DEFERRED_ROLE firmware)
    set_property(GLOBAL PROPERTY FCI_PROTOCOL_DEFERRED_COMPONENTS
        "${FCI_PROTOCOL_WIRELINK_COMPONENTS}")
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/Zephyr.cmake)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/FciProtocolWirelink.cmake)
if(ZEPHYR_BASE)
    # Zephyr modules are not ordered by dependency. Defer generation until
    # every module has created its targets. Applications may still name the
    # eventual fci_protocol::<component> target in target_link_libraries().
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
        CALL fci_protocol_enable_wirelink)
else()
    fci_protocol_enable_wirelink()
endif()

if(FCI_PROTOCOL_BUILD_SCHEMA_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
