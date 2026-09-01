add_library(florid_wirelink_host
    WirelinkExecutor.cpp
    FciWirelinkEndpoint.cpp
)
add_library(florid::wirelink_host ALIAS florid_wirelink_host)

target_include_directories(florid_wirelink_host
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)
target_compile_features(florid_wirelink_host PUBLIC cxx_std_20)
target_link_libraries(florid_wirelink_host
    PUBLIC
        fci_protocol::arm
    PRIVATE
        Threads::Threads
)

add_library(florid
    tick_std.cpp
    timestamp_std.cpp
    AstrialUSBTransport.cpp
    UdpTransport.cpp
    ArmImpl.cpp
    Arm.cpp
    Gripper.cpp
    LatencyEstimator.cpp
)

target_link_libraries(florid
    PUBLIC
        astrial
    PRIVATE
        asio_standalone
        florid_wirelink_host
)

target_include_directories(florid
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_compile_features(florid PUBLIC cxx_std_20)

if(BUILD_MPC)
    target_include_directories(florid PUBLIC
        "${PROJECT_SOURCE_DIR}/generated"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code"
    )
    target_include_directories(florid PRIVATE
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_model"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_cost"
        "${ACADOS_SOURCE_DIR}/include"
        "${ACADOS_SOURCE_DIR}/interfaces"
        "${ACADOS_SOURCE_DIR}/external/blasfeo/include"
        "${ACADOS_SOURCE_DIR}/external/hpipm/include"
    )
    target_link_libraries(florid PUBLIC acados)

    target_sources(florid PRIVATE
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/acados_solver_willow_mpc.c"
        "${PROJECT_SOURCE_DIR}/generated/c_generated_code/willow_mpc_model/willow_mpc_expl_ode_fun.c"
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
