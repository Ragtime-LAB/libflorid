#pragma once
// ---------------------------------------------------------
// urdf2mpc.py auto-generated — DO NOT EDIT!
// DOF:6  H:5  dt:4ms  nx:12  nu:6
//
// 此文件仅包含轻量 C++ traits 封装。
// 实际 C 实现文件位于:
//   generated/c_generated_code/acados_solver_willow_mpc.c
//   generated/c_generated_code/willow_mpc_model/*.c
//   generated/c_generated_code/willow_mpc_cost/*.c
// ---------------------------------------------------------

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>

// acados solver wrapper (manually generated, compiled as separate .c)
extern "C" {
#include "acados_solver_willow_mpc.h"
}

namespace florid {

struct WillowMPCTraits {
    static constexpr int kNX      = 12;
    static constexpr int kNU      = 6;
    static constexpr int kDOF     = 6;
    static constexpr int kHorizon = 5;
    static constexpr float kDt    = 0.004f;

    static constexpr float kQLower[6]   = {-3.14f, 0.0f, 0.0f, -1.3f, -1.57f, -1.57f};
    static constexpr float kQUpper[6]   = {3.14f, 3.14f, 3.14f, 1.3f, 1.57f, 1.57f};
    static constexpr float kTauLimit[6] = {5.0f, 5.0f, 5.0f, 3.0f, 3.0f, 3.0f};
    static constexpr float kDqLimit[6]  = {12.48f, 3.744f, 3.744f, 12.48f, 12.48f, 12.48f};

    static void* create() {
        auto* cap = willow_mpc_acados_create_capsule();
        willow_mpc_acados_create(cap);
        return cap;
    }

    static void destroy(void* cap) {
        auto* c = static_cast<willow_mpc_solver_capsule*>(cap);
        willow_mpc_acados_free(c);
        willow_mpc_acados_free_capsule(c);
    }

    static void setInitialState(void* cap, const float* x0) {
        auto* c = static_cast<willow_mpc_solver_capsule*>(cap);
        ocp_nlp_constraints_model_set(c->nlp_config, c->nlp_dims, c->nlp_in, c->nlp_out,
            0, "lbx", const_cast<float*>(x0));
        ocp_nlp_constraints_model_set(c->nlp_config, c->nlp_dims, c->nlp_in, c->nlp_out,
            0, "ubx", const_cast<float*>(x0));
    }

    static void setReference(void* cap, const float* pos_ref) {
        float yref[15] = {};
        yref[0] = pos_ref[0]; yref[1] = pos_ref[1]; yref[2] = pos_ref[2];
        float yref_e[9] = {};
        yref_e[0] = pos_ref[0]; yref_e[1] = pos_ref[1]; yref_e[2] = pos_ref[2];

        auto* c = static_cast<willow_mpc_solver_capsule*>(cap);
        for (int i = 0; i < 5; ++i)
            ocp_nlp_cost_model_set(c->nlp_config, c->nlp_dims, c->nlp_in, i, "yref", yref);
        ocp_nlp_cost_model_set(c->nlp_config, c->nlp_dims, c->nlp_in, 5, "yref", yref_e);
    }

    static int solve(void* cap) {
        return willow_mpc_acados_solve(static_cast<willow_mpc_solver_capsule*>(cap));
    }

    static void getOptimalQ(void* cap, float* q_out) {
        auto* c = static_cast<willow_mpc_solver_capsule*>(cap);
        float x_traj[12];
        ocp_nlp_out_get(c->nlp_config, c->nlp_dims, c->nlp_out, 0, "x", x_traj);
        std::memcpy(q_out, x_traj, 6 * sizeof(float));
    }

    static void getOptimalDq(void* cap, float* dq_out) {
        auto* c = static_cast<willow_mpc_solver_capsule*>(cap);
        float x_traj[12];
        ocp_nlp_out_get(c->nlp_config, c->nlp_dims, c->nlp_out, 0, "x", x_traj);
        std::memcpy(dq_out, &x_traj[6], 6 * sizeof(float));
    }
};

} // namespace florid
