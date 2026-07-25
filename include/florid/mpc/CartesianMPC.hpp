#ifndef FLORID_MPC_CARTESIAN_MPC_HPP
#define FLORID_MPC_CARTESIAN_MPC_HPP

#include "florid/ControlTypes.hpp"
#include <memory>

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

    JointPVT solve(const float* q, const float* dq, const float* T_ref);
    JointPVT fallbackIK(const float* q, const float* T_ref);

private:
    void* m_capsule;
    MPCConfig m_cfg;
};

// ── 模板实现 ──

template <typename MPCTraits>
CartesianMPCSolver<MPCTraits>::CartesianMPCSolver(const MPCConfig& cfg)
    : m_capsule(MPCTraits::create())
    , m_cfg(cfg)
{}

template <typename MPCTraits>
CartesianMPCSolver<MPCTraits>::~CartesianMPCSolver() {
    if (m_capsule) {
        MPCTraits::destroy(m_capsule);
    }
}

template <typename MPCTraits>
JointPVT CartesianMPCSolver<MPCTraits>::solve(const float* q, const float* dq, const float* T_ref) {
    JointPVT s_out{};

    float x0[MPCTraits::kNX];
    std::memcpy(x0,        q,  MPCTraits::kDOF * sizeof(float));
    std::memcpy(x0 + MPCTraits::kDOF, dq, MPCTraits::kDOF * sizeof(float));

    MPCTraits::setInitialState(m_capsule, x0);

    float pos_ref[3]{ T_ref[12], T_ref[13], T_ref[14] };
    MPCTraits::setReference(m_capsule, pos_ref);

    int status = MPCTraits::solve(m_capsule);
    if (status != 0) {
        return fallbackIK(q, T_ref);
    }

    MPCTraits::getOptimalQ(m_capsule, s_out.m_q);
    MPCTraits::getOptimalDq(m_capsule, s_out.m_dq_limit);

    for (int i = 0; i < MPCTraits::kDOF; ++i) {
        s_out.m_dq_limit[i] = MPCTraits::kDqLimit[i] * m_cfg.velocity_excitation;
        s_out.m_current_limit_norm[i] = m_cfg.current_limit_norm;
    }

    return s_out;
}

template <typename MPCTraits>
JointPVT CartesianMPCSolver<MPCTraits>::fallbackIK(const float* q, const float* T_ref) {
    JointPVT s_out{};
    std::memcpy(s_out.m_q, q, MPCTraits::kDOF * sizeof(float));
    for (int i = 0; i < MPCTraits::kDOF; ++i) {
        s_out.m_dq_limit[i] = MPCTraits::kDqLimit[i] * m_cfg.velocity_excitation;
        s_out.m_current_limit_norm[i] = m_cfg.current_limit_norm;
    }
    return s_out;
}

} // namespace florid

#endif
