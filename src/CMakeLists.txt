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
    UdpTransport.cpp
    ArmImpl.cpp
    Arm.cpp
    Gripper.cpp
    FirmwareUpdater.cpp
    FirmwareUpdateSession.cpp
    LatencyEstimator.cpp
)

target_link_libraries(florid
    PUBLIC
        wirelink::astrial
        wirelink::astrial_usb
        wirelink::asio_udp
    PRIVATE
        florid_wirelink_host
)

target_include_directories(florid
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_compile_features(florid PUBLIC cxx_std_20)

# Clang/libstdc++ may implement even lock-free queries in libatomic. Probe
# with the selected compiler/flags, and propagate the library for inline SDK
# consumers such as ControlGeneration as well as MPCSession.cpp.
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

if(BUILD_MPC)
    if(WIN32)
        target_link_libraries(florid PRIVATE winmm)
    endif()
    target_compile_definitions(florid PUBLIC FLORID_HAS_MPC)
    target_include_directories(florid PUBLIC
        "${PROJECT_SOURCE_DIR}/generated"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code"
    )
    target_include_directories(florid PRIVATE
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_model"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost"
    )
    target_link_libraries(florid PUBLIC florid::acados)

    target_sources(florid PRIVATE
        MPCSession.cpp
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/acados_solver_willow_mpc.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_model/willow_mpc_expl_ode_fun.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_model/willow_mpc_gravity.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_model/willow_mpc_expl_vde_adj.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_model/willow_mpc_expl_vde_forw.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost/willow_mpc_cost_y_fun.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost/willow_mpc_cost_y_fun_jac_ut_xt.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost/willow_mpc_cost_y_hess.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost/willow_mpc_cost_y_e_fun.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost/willow_mpc_cost_y_e_fun_jac_ut_xt.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost/willow_mpc_cost_y_e_hess.c"
    )
endif()
