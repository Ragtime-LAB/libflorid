#include "florid/Model.hpp"
#include "florid/traits/WillowTraits.hpp"
#include "florid/mpc/CartesianMPC.hpp"
#include "WillowMPCTraits.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <type_traits>

extern "C" int willow_mpc_cost_y_fun(const double**, double**, int*, double*, int);
extern "C" int willow_mpc_expl_ode_fun(const double**, double**, int*, double*, int);

namespace {
using Traits = florid::WillowMPCTraits;
using Solver = florid::CartesianMPCSolver<Traits>;
static_assert(!std::is_copy_constructible_v<Solver>);
static_assert(!std::is_move_constructible_v<Solver>);

void require(bool s_ok, const char* s_message) {
    if (!s_ok) throw std::runtime_error(s_message);
}

std::array<float, 16> pose(const float* s_q) {
    std::array<float, 16> s_T{};
    florid::Model<florid::WillowTraits>{}.forwardKinematics(s_q, s_T.data());
    return s_T;
}

double error(const float* s_q, const float* s_target) {
    const auto s_T = pose(s_q);
    double s_sum = 0;
    for (int i = 12; i < 15; ++i) s_sum += std::pow(s_T[i] - s_target[i], 2);
    return std::sqrt(s_sum);
}

void testModelConsistency() {
    float s_q[6] = {0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f};
    float s_dq[6] = {0.1f, -0.1f, 0.2f, 0, 0.1f, 0};
    double s_x[12], s_u[6] = {0.5, -0.5, 0.3, 0.1, 0, 0.1};
    for (int i = 0; i < 6; ++i) { s_x[i] = s_q[i]; s_x[i+6] = s_dq[i]; }
    const double* s_args[5] = {s_x, s_u, nullptr, nullptr, nullptr};
    double s_y[15], s_dx[12];
    double* s_result[] = {s_y};
    willow_mpc_cost_y_fun(s_args, s_result, nullptr, nullptr, 0);
    const auto s_T = pose(s_q);
    for (int i = 0; i < 3; ++i) {
        require(std::abs(s_y[i] - s_T[12+i]) < 1e-6, "MPC and SDK forward kinematics differ");
    }
    s_result[0] = s_dx;
    willow_mpc_expl_ode_fun(s_args, s_result, nullptr, nullptr, 0);
    float s_M[36], s_c[6], s_g[6];
    const float s_gravity[3] = {0, 0, -9.81f};
    florid::Model<florid::WillowTraits> s_model;
    s_model.mass(s_q, s_M);
    s_model.coriolis(s_q, s_dq, s_c);
    s_model.gravity(s_q, s_gravity, s_g);
    double s_gravity_reference[6];
    willow_mpc_acados_gravity(s_x, s_gravity_reference);
    for (int i = 0; i < 6; ++i) {
        require(std::abs(s_g[i] - s_gravity_reference[i]) < 1e-5,
                "MPC gravity reference differs from SDK model");
        double s_tau = s_c[i] + s_g[i];
        for (int j = 0; j < 6; ++j) s_tau += s_M[6*i+j] * s_dx[6+j];
        require(std::abs(s_tau - s_u[i]) < 1e-4, "MPC and SDK dynamics differ");
    }
}

void testStationaryReference() {
    Solver s_solver;
    float s_q[6] = {0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f};
    float s_dq[6]{};
    const auto s_target = pose(s_q);
    const std::array<float, 6> s_initial{0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f};
    for (int i = 0; i < 200; ++i) {
        const auto s_cmd = s_solver.solve(s_q, s_dq, s_target.data());
        require(s_solver.lastStatus() == 0, "stationary solve failed");
        for (int j = 0; j < 6; ++j) {
            require(std::abs(s_cmd.m_q[j] - s_initial[j]) < 1e-4,
                    "stationary reference drifted");
            s_dq[j] = (s_cmd.m_q[j] - s_q[j]) / Traits::kDt;
            s_q[j] = s_cmd.m_q[j];
        }
    }
}

void testLifecycleAndPrediction() {
    for (int trial = 0; trial < 10; ++trial) {
        auto* s_cap = static_cast<willow_mpc_solver_capsule*>(Traits::create());
        float s_x[12] = {0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f,
                        0.1f, 0, 0, 0, 0, 0};
        const auto s_T = pose(s_x);
        Traits::setInitialState(s_cap, s_x);
        Traits::setReference(s_cap, &s_T[12]);
        require(Traits::solve(s_cap) == 0, "acados failed on a feasible state");
        double s_x0[12];
        ocp_nlp_out_get(s_cap->nlp_config, s_cap->nlp_dims, s_cap->nlp_out, 0, "x", s_x0);
        for (int j = 0; j < 12; ++j)
            require(std::abs(s_x0[j] - s_x[j]) < 1e-7, "initial state lost precision");

        struct Guarded {
            float before = 123.0f;
            float data[6]{};
            float after = 456.0f;
        } s_q, s_dq;
        Traits::getOptimalQ(s_cap, s_q.data);
        Traits::getOptimalDq(s_cap, s_dq.data);
        require(s_q.before == 123 && s_q.after == 456 &&
                s_dq.before == 123 && s_dq.after == 456, "result buffer overwritten");
        double s_x1[12];
        ocp_nlp_out_get(s_cap->nlp_config, s_cap->nlp_dims, s_cap->nlp_out, 1, "x", s_x1);
        for (int j = 0; j < 6; ++j) {
            require(std::abs(s_q.data[j] - s_x1[j]) < 1e-6, "q must come from stage 1");
            require(std::abs(s_dq.data[j] - s_x1[6+j]) < 1e-6, "dq must come from stage 1");
        }
        require(std::abs(s_q.data[0] - s_x[0]) > 1e-5f, "prediction did not advance time");
        for (int i = 1; i <= Traits::kHorizon; ++i) {
            double s_xi[12];
            ocp_nlp_out_get(s_cap->nlp_config, s_cap->nlp_dims, s_cap->nlp_out, i, "x", s_xi);
            for (int j = 0; j < 6; ++j) {
                require(std::isfinite(s_xi[j]) && s_xi[j] >= Traits::kQLower[j] - 1e-5 &&
                        s_xi[j] <= Traits::kQUpper[j] + 1e-5, "position bound violated");
                require(std::isfinite(s_xi[6+j]) &&
                        std::abs(s_xi[6+j]) <= Traits::kDqLimit[j] + 1e-5,
                        "velocity bound violated");
            }
        }
        for (int i = 0; i < Traits::kHorizon; ++i) {
            double s_u[6];
            ocp_nlp_out_get(s_cap->nlp_config, s_cap->nlp_dims, s_cap->nlp_out, i, "u", s_u);
            for (int j = 0; j < 6; ++j)
                require(std::isfinite(s_u[j]) && std::abs(s_u[j]) <= Traits::kTauLimit[j] + 1e-5,
                        "torque bound violated");
        }
        Traits::destroy(s_cap);
    }
}

void testPositionTracking(std::array<float, 6> s_initial_q,
                          const std::array<float, 6>& s_goal) {
    Solver s_solver;
    float* s_q = s_initial_q.data();
    float s_dq[6]{};
    const auto s_target = pose(s_goal.data());
    const double s_initial = error(s_q, s_target.data());
    // Ideal joint-position servo: exercise the actual JointPVT command path.
    for (int i = 0; i < 500; ++i) {
        const auto s_cmd = s_solver.solve(s_q, s_dq, s_target.data());
        if (s_solver.lastStatus() != 0)
            std::printf("tracking failed at step %d, status %d\n", i, s_solver.lastStatus());
        require(s_solver.lastStatus() == 0, "position tracking fell back to hold");
        for (int j = 0; j < 6; ++j) {
            require(s_cmd.m_q[j] >= Traits::kQLower[j] && s_cmd.m_q[j] <= Traits::kQUpper[j],
                    "PVT position out of range");
            require(s_cmd.m_dq_limit[j] > 0 && s_cmd.m_dq_limit[j] <= Traits::kDqLimit[j],
                    "PVT speed ceiling out of range");
            s_dq[j] = (s_cmd.m_q[j] - s_q[j]) / Traits::kDt;
            s_q[j] = s_cmd.m_q[j];
        }
    }
    const double s_final = error(s_q, s_target.data());
    std::printf("position tracking: %.6f -> %.6f m\n", s_initial, s_final);
    require(s_final < 1e-4 && s_final < s_initial * 0.01, "MPC did not reduce Cartesian position error");
}

struct NonfinitePrediction : Traits {
    static void getOptimalQ(void*, float* s_q) {
        for (int i = 0; i < 6; ++i) s_q[i] = std::numeric_limits<float>::quiet_NaN();
    }
};

void testInvalidPrediction() {
    florid::CartesianMPCSolver<NonfinitePrediction> s_solver;
    float s_q[6] = {0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f};
    float s_dq[6]{};
    const auto s_target = pose(s_q);
    const auto s_hold = s_solver.solve(s_q, s_dq, s_target.data());
    require(s_solver.lastStatus() == -2, "nonfinite prediction reported success");
    for (int i = 0; i < 6; ++i)
        require(s_hold.m_q[i] == s_q[i], "nonfinite prediction reached command output");
}

struct NonfiniteTerminalPrediction : Traits {
    static void getState(void* s_cap, int s_stage, double* s_x) {
        Traits::getState(s_cap, s_stage, s_x);
        if (s_stage == kHorizon) s_x[7] = std::numeric_limits<double>::quiet_NaN();
    }
};

void testFullPrediction() {
    float s_q[6]{0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f}, s_dq[6]{};
    const auto s_target = pose(s_q);
    Solver s_solver;
    s_solver.setVelocityLimit(0.5f);
    Solver::Prediction s_prediction;
    require(s_solver.predict(s_q, s_dq, s_target.data(), s_prediction), "full prediction failed");
    for (const auto& s_knot : s_prediction)
        for (int i = 0; i < 6; ++i) {
            require(std::abs(s_knot.m_q[i] - s_q[i]) < 1e-4 &&
                    std::abs(s_knot.m_dq[i]) <= 0.5f, "full hold trajectory is invalid");
        }
    s_solver.resetWarmStart();
    require(s_solver.predict(s_q, s_dq, s_target.data(), s_prediction), "solve after warm-start reset failed");
    florid::CartesianMPCSolver<NonfiniteTerminalPrediction> s_bad;
    require(!s_bad.predict(s_q, s_dq, s_target.data(), s_prediction) && s_bad.lastStatus() == -2,
            "nonfinite terminal state published as a valid trajectory");
}

void testInvalidInputAndRecovery() {
    Solver s_solver;
    float s_q[6] = {0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f};
    float s_dq[6]{};
    auto s_target = pose(s_q);
    const auto s_valid = s_target;
    s_target[12] = std::numeric_limits<float>::quiet_NaN();
    bool s_rejected = false;
    try { s_solver.solve(s_q, s_dq, s_target.data()); }
    catch (const florid::CommandException&) { s_rejected = true; }
    require(s_rejected, "NaN target accepted");
    s_target = s_valid;
    s_target[3] = 0.5f;
    s_rejected = false;
    try { s_solver.solve(s_q, s_dq, s_target.data()); }
    catch (const florid::CommandException&) { s_rejected = true; }
    require(s_rejected, "row-major target accepted");

    // This state cannot reach the joint bounds within one 20 ms interval.
    s_q[1] = 10.0f;
    const auto s_hold = s_solver.solve(s_q, s_dq, s_valid.data());
    require(s_solver.lastStatus() != 0, "infeasible state reported success");
    for (int j = 0; j < 6; ++j)
        require(s_hold.m_q[j] == s_q[j], "failed solve did not hold measured position");
    s_q[1] = 1.5f;
    s_solver.solve(s_q, s_dq, s_valid.data());
    require(s_solver.lastStatus() == 0, "solver did not recover after infeasibility");
}
} // namespace

int main() {
    try {
        testModelConsistency();
        testLifecycleAndPrediction();
        testStationaryReference();
        testPositionTracking({0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f},
                             {0.15f, 1.48f, 0.32f, 0.1f, 0.2f, 0.1f});
        testPositionTracking({0.2f, 1.4f, 0.2f, -0.1f, -0.2f, 0.1f},
                             {0.15f, 1.42f, 0.18f, -0.1f, -0.2f, 0.1f});
        testPositionTracking({-0.2f, 1.45f, 0.15f, -0.2f, 0.3f, 0.1f},
                             {-0.15f, 1.43f, 0.17f, -0.2f, 0.3f, 0.1f});
        testInvalidInputAndRecovery();
        testInvalidPrediction();
        testFullPrediction();
    } catch (const std::exception& s_error) {
        std::fprintf(stderr, "%s\n", s_error.what());
        return 1;
    }
}
