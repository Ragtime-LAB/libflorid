add_library(florid_wirelink_host
    FciWirelinkEndpoint.cpp
    FciUpgradeClient.cpp
)
add_library(florid::wirelink_host ALIAS florid_wirelink_host)

target_include_directories(florid_wirelink_host
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)
target_compile_features(florid_wirelink_host PUBLIC cxx_std_20)
target_link_libraries(florid_wirelink_host
    PUBLIC
        fci_protocol::device
        Wirelink::host
        Wirelink::platform
    PRIVATE
        Threads::Threads
)

add_library(florid
    AstrialBulkTransport.cpp
    AstrialUSBTransport.cpp
    DeviceDiscovery.cpp
    UsbDiscovery.cpp
    FirmwareUpdateSession.cpp
    UnderwaterTeleop.cpp
)

target_link_libraries(florid
    PUBLIC
        wirelink::astrial
        wirelink::astrial_usb
    PRIVATE
        florid_wirelink_host
)

target_include_directories(florid
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_compile_features(florid PUBLIC cxx_std_20)

# Clang/libstdc++ may require libatomic for the 64-bit Control session token.
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)
cmake_push_check_state(RESET)
set(_lf_atomic_probe "
#include <atomic>
#include <cstdint>
int main() {
    std::atomic<std::uint64_t> generation{0};
    std::atomic<unsigned> inflight{0};
    std::atomic<bool> active{false};
    return generation.is_lock_free() && inflight.is_lock_free() && active.is_lock_free() ? 0 : 1;
}")
check_cxx_source_compiles("${_lf_atomic_probe}" LF_ATOMICS_LINK_WITHOUT_LIBRARY)
if(NOT LF_ATOMICS_LINK_WITHOUT_LIBRARY)
    set(CMAKE_REQUIRED_LIBRARIES atomic)
    check_cxx_source_compiles("${_lf_atomic_probe}" LF_ATOMICS_LINK_WITH_LIBATOMIC)
    if(NOT LF_ATOMICS_LINK_WITH_LIBATOMIC)
        message(FATAL_ERROR "The selected compiler needs a working C++ atomic runtime")
    endif()
    target_link_libraries(florid PUBLIC atomic)
endif()
cmake_pop_check_state()
unset(_lf_atomic_probe)
