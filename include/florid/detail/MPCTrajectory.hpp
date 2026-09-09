#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace florid::detail {
struct MPCJointState {
    std::array<double, 6> m_q{}, m_dq{};
};

struct MPCJointLimits {
    std::array<double, 6> m_lower{}, m_upper{}, m_velocity{}, m_acceleration{};
};

// Cubic Hermite in normalized time. Exact extrema checks include the interior,
// where position/velocity may exceed both endpoint values.
class MPCCubic {
public:
    MPCCubic(const MPCJointState& s_from, const MPCJointState& s_to, double s_dt)
        : m_dt(s_dt) {
        for (int i = 0; i < 6; ++i) {
            const double s_delta = s_to.m_q[i] - s_from.m_q[i];
            m_a[i] = -2 * s_delta + s_dt * (s_from.m_dq[i] + s_to.m_dq[i]);
            m_b[i] = 3 * s_delta - s_dt * (2 * s_from.m_dq[i] + s_to.m_dq[i]);
            m_c[i] = s_dt * s_from.m_dq[i];
            m_d[i] = s_from.m_q[i];
        }
    }

    double duration() const noexcept { return m_dt; }

    MPCJointState sample(double s_time) const {
        const double s_u = std::clamp(s_time / m_dt, 0.0, 1.0);
        MPCJointState s_out;
        for (int i = 0; i < 6; ++i) {
            s_out.m_q[i] = position(i, s_u);
            s_out.m_dq[i] = velocity(i, s_u);
        }
        return s_out;
    }

    bool within(const MPCJointLimits& s_limits) const {
        if (!std::isfinite(m_dt) || m_dt <= 0) return false;
        for (int i = 0; i < 6; ++i) {
            if (!std::isfinite(m_a[i]) || !std::isfinite(m_b[i]) ||
                !std::isfinite(m_c[i]) || !std::isfinite(m_d[i])) return false;
            auto s_position_ok = [&](double s_u) {
                const auto s_q = position(i, s_u);
                return s_q >= s_limits.m_lower[i] - 1e-7 &&
                       s_q <= s_limits.m_upper[i] + 1e-7;
            };
            auto s_velocity_ok = [&](double s_u) {
                return std::abs(velocity(i, s_u)) <= s_limits.m_velocity[i] + 1e-7;
            };
            if (!s_position_ok(0) || !s_position_ok(1) ||
                !s_velocity_ok(0) || !s_velocity_ok(1)) return false;
            if (std::max(std::abs(2 * m_b[i]), std::abs(6 * m_a[i] + 2 * m_b[i])) /
                    (m_dt * m_dt) > s_limits.m_acceleration[i] + 1e-7) return false;
            if (std::abs(m_a[i]) > 1e-15) {
                const double s_root = -m_b[i] / (3 * m_a[i]);
                if (s_root > 0 && s_root < 1 && !s_velocity_ok(s_root)) return false;
                const double s_disc = m_b[i]*m_b[i] - 3*m_a[i]*m_c[i];
                if (s_disc >= 0) {
                    for (double s_sign : {-1.0, 1.0}) {
                        const double s_u = (-m_b[i] + s_sign * std::sqrt(s_disc)) / (3*m_a[i]);
                        if (s_u > 0 && s_u < 1 && !s_position_ok(s_u)) return false;
                    }
                }
            } else if (std::abs(m_b[i]) > 1e-15) {
                const double s_u = -m_c[i] / (2*m_b[i]);
                if (s_u > 0 && s_u < 1 && !s_position_ok(s_u)) return false;
            }
        }
        return true;
    }

private:
    double position(int i, double u) const { return ((m_a[i]*u + m_b[i])*u + m_c[i])*u + m_d[i]; }
    double velocity(int i, double u) const { return ((3*m_a[i]*u + 2*m_b[i])*u + m_c[i]) / m_dt; }
    std::array<double, 6> m_a{}, m_b{}, m_c{}, m_d{};
    double m_dt;
};

template <std::size_t N>
MPCJointState sampleMPC(const std::array<MPCJointState, N>& s_knots,
                       double s_dt, double s_time) {
    const auto s_t = std::clamp(s_time, 0.0, s_dt * (N - 1));
    const auto s_i = std::min(static_cast<std::size_t>(s_t / s_dt), N - 2);
    return MPCCubic(s_knots[s_i], s_knots[s_i + 1], s_dt).sample(s_t - s_i*s_dt);
}

// Try at most eleven C1 handoffs on the original prediction time axis. A longer
// duration moves the destination too, so each candidate needs its own check.
template <std::size_t N>
std::optional<MPCCubic> makeMPCTransition(const MPCJointState& s_from,
                                       const std::array<MPCJointState, N>& s_knots,
                                       double s_dt, double s_age, double s_plan_timeout,
                                       const MPCJointLimits& s_limits) {
    static_assert(N >= 2);
    if (!std::isfinite(s_dt) || s_dt <= 0 || !std::isfinite(s_age) || s_age < 0 ||
        !std::isfinite(s_plan_timeout) || s_plan_timeout <= 0) return std::nullopt;
    const double s_remaining = std::min(s_dt * (N - 1), s_plan_timeout) - s_age;
    for (int s_ms = 20; s_ms <= 40; s_ms += 2) {
        const double s_duration = s_ms * 0.001;
        if (s_duration > s_remaining) break;
        const auto s_to = sampleMPC(s_knots, s_dt, s_age + s_duration);
        MPCCubic s_curve(s_from, s_to, s_duration);
        if (s_curve.within(s_limits)) return s_curve;
    }
    return std::nullopt;
}
} // namespace florid::detail
