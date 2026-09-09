#ifndef FLORID_MPC_CARTESIAN_MPC_HPP
#define FLORID_MPC_CARTESIAN_MPC_HPP

#include "florid/ControlTypes.hpp"
#include "florid/Exceptions.hpp"
#include "florid/detail/MPCTrajectory.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace florid {

struct MPCConfig {
    float current_limit_norm = 0.3f;
    float velocity_excitation = 1.2f;
};

template <typename MPCTraits>
class CartesianMPCSolver {
public:
    explicit CartesianMPCSolver(const MPCConfig& cfg = MPCConfig{});
    ~CartesianMPCSolver();

    CartesianMPCSolver(const CartesianMPCSolver&) = delete;
    CartesianMPCSolver& operator=(const CartesianMPCSolver&) = delete;
    CartesianMPCSolver(CartesianMPCSolver&&) = delete;
    CartesianMPCSolver& operator=(CartesianMPCSolver&&) = delete;

    // Position-only MPC. T_ref is column-major, translation at [12..14].
    // Rotation and Cartesian impedance gains are not part of this OCP.
    JointPVT solve(const float* q, const float* dq, const float* T_ref);
    using Prediction = std::array<detail::MPCJointState, MPCTraits::kHorizon + 1>;
    // The multi-rate controller uses the full trajectory. A failed solve never
    // publishes a hold command as a successful prediction.
    bool predict(const float* q, const float* dq, const float* T_ref, Prediction& s_out) {
        (void)solve(q, dq, T_ref);
        if (m_status != 0) return false;
        for (int s_stage = 0; s_stage <= MPCTraits::kHorizon; ++s_stage) {
            double s_x[MPCTraits::kNX];
            MPCTraits::getState(m_capsule, s_stage, s_x);
            for (int i = 0; i < MPCTraits::kDOF; ++i) {
                if (!std::isfinite(s_x[i]) || !std::isfinite(s_x[6+i]) ||
                    s_x[i] < MPCTraits::kQLower[i] - 1e-5 ||
                    s_x[i] > MPCTraits::kQUpper[i] + 1e-5 ||
                    std::abs(s_x[6+i]) > MPCTraits::kDqLimit[i] + 1e-5) {
                    m_status = -2;
                    resetWarmStart();
                    return false;
                }
                s_out[s_stage].m_q[i] = std::clamp(s_x[i],
                    double(MPCTraits::kQLower[i]), double(MPCTraits::kQUpper[i]));
                s_out[s_stage].m_dq[i] = s_x[6+i];
            }
        }
        return true;
    }
    void setVelocityLimit(float s_limit) {
        if (!std::isfinite(s_limit) || s_limit <= 0)
            throw CommandException("MPC velocity limit must be positive and finite");
        MPCTraits::setVelocityLimit(m_capsule, s_limit);
    }
    void resetWarmStart() { MPCTraits::resetWarmStart(m_capsule); }
    JointPVT holdPosition(const float* q) const;
    [[deprecated("This is a position hold, not inverse kinematics; use holdPosition")]]
    JointPVT fallbackIK(const float* q, const float* T_ref);
    // 0: success; positive: acados failure; -1: not run; -2: invalid solution.
    int lastStatus() const noexcept { return m_status; }

private:
    void* m_capsule = nullptr;
    MPCConfig m_cfg;
    int m_status = -1;
};

// ── 模板实现 ──

template <typename MPCTraits>
CartesianMPCSolver<MPCTraits>::CartesianMPCSolver(const MPCConfig& cfg)
    : m_cfg(cfg) {
    static_assert(MPCTraits::kDOF == 6 && MPCTraits::kNX == 12);
    if (!std::isfinite(cfg.current_limit_norm) || cfg.current_limit_norm <= 0.0f ||
        cfg.current_limit_norm > 1.0f || !std::isfinite(cfg.velocity_excitation) ||
        cfg.velocity_excitation < 1.0f) {
        throw CommandException("Invalid MPC current limit or velocity excitation");
    }
    m_capsule = MPCTraits::create();
    if (!m_capsule) throw ControlException("MPC initialization returned no solver");
}

template <typename MPCTraits>
CartesianMPCSolver<MPCTraits>::~CartesianMPCSolver() {
    if (m_capsule) {
        MPCTraits::destroy(m_capsule);
    }
}

template <typename MPCTraits>
JointPVT CartesianMPCSolver<MPCTraits>::solve(const float* q, const float* dq, const float* T_ref) {
    JointPVT s_out{};
    if (!q || !dq || !T_ref) throw CommandException("MPC inputs must not be null");
    for (int i = 0; i < MPCTraits::kDOF; ++i) {
        if (!std::isfinite(q[i]) || !std::isfinite(dq[i]))
            throw CommandException("MPC joint state must be finite");
    }
    for (int i = 0; i < 16; ++i) {
        if (!std::isfinite(T_ref[i]))
            throw CommandException("MPC target must be finite");
    }
    if (std::abs(T_ref[3]) > 1e-5f || std::abs(T_ref[7]) > 1e-5f ||
        std::abs(T_ref[11]) > 1e-5f || std::abs(T_ref[15] - 1.0f) > 1e-5f)
        throw CommandException("MPC target must be a column-major homogeneous transform");

    float x0[MPCTraits::kNX];
    std::memcpy(x0,        q,  MPCTraits::kDOF * sizeof(float));
    std::memcpy(x0 + MPCTraits::kDOF, dq, MPCTraits::kDOF * sizeof(float));

    MPCTraits::setInitialState(m_capsule, x0);

    float pos_ref[3]{ T_ref[12], T_ref[13], T_ref[14] };
    MPCTraits::setReference(m_capsule, pos_ref);

    m_status = MPCTraits::solve(m_capsule);
    if (m_status != 0) {
        return holdPosition(q);
    }

    MPCTraits::getOptimalQ(m_capsule, s_out.m_q);
    float s_dq[MPCTraits::kDOF];
    MPCTraits::getOptimalDq(m_capsule, s_dq);

    for (int i = 0; i < MPCTraits::kDOF; ++i) {
        if (!std::isfinite(s_out.m_q[i]) || !std::isfinite(s_dq[i]) ||
            s_out.m_q[i] < MPCTraits::kQLower[i] - 1e-4f ||
            s_out.m_q[i] > MPCTraits::kQUpper[i] + 1e-4f ||
            std::abs(s_dq[i]) > MPCTraits::kDqLimit[i] + 1e-4f) {
            m_status = -2;
            resetWarmStart();
            return holdPosition(q);
        }
        s_out.m_q[i] = std::clamp(s_out.m_q[i], MPCTraits::kQLower[i], MPCTraits::kQUpper[i]);
        // JointPVT expects a nonnegative speed ceiling, not signed velocity.
        const float s_speed = std::max(std::abs(s_dq[i]),
            std::abs(s_out.m_q[i] - q[i]) / MPCTraits::kDt);
        s_out.m_dq_limit[i] = std::min(MPCTraits::kDqLimit[i],
            std::max(s_speed * m_cfg.velocity_excitation, 1e-4f));
        s_out.m_current_limit_norm[i] = m_cfg.current_limit_norm;
    }

    return s_out;
}

template <typename MPCTraits>
JointPVT CartesianMPCSolver<MPCTraits>::holdPosition(const float* q) const {
    JointPVT s_out{};
    if (!q) throw CommandException("MPC hold state must not be null");
    std::memcpy(s_out.m_q, q, MPCTraits::kDOF * sizeof(float));
    for (int i = 0; i < MPCTraits::kDOF; ++i) {
        if (!std::isfinite(q[i])) throw CommandException("MPC hold state must be finite");
        s_out.m_dq_limit[i] = MPCTraits::kDqLimit[i];
        s_out.m_current_limit_norm[i] = m_cfg.current_limit_norm;
    }
    return s_out;
}

template <typename MPCTraits>
JointPVT CartesianMPCSolver<MPCTraits>::fallbackIK(const float* q, const float*) {
    return holdPosition(q);
}

} // namespace florid

#endif
